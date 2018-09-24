#include "public_def.h"
#include "private_def.h"



static void close_cb(uv_handle_t* handle) {
    socket_t* socket = (socket_t*)handle->data;
    socket->status = socket_closed;
    uv_mutex_destroy(&socket->uv_mutex_h);
    free(socket);
}

static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    socket_t* socket = (socket_t*)handle->data;
    memset(socket->buff, 0, SOCKET_RECV_BUFF_LEN);
    *buf = uv_buf_init(socket->buff, SOCKET_RECV_BUFF_LEN);
}

static void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    socket_t* socket = (socket_t*)stream->data;
    bool_t finish = false;
	uv_mutex_lock(&socket->uv_mutex_h);
    if (nread < 0) {
        int code;
        if (nread == UV_EOF) {
            fprintf(stderr, "server close this socket\r\n");
            code = uv_http_err_remote_disconnect;
        } else {
            fprintf(stderr, "read_cb error %s-%s\r\n", uv_err_name(nread), uv_strerror(nread));
            code = uv_http_err_local_disconnect;
        }

        if (socket->status == socket_send) {
            if (socket->req->res == NULL) {
                socket->req->res = create_response(socket->req);
            }
            response_error(socket->req->res, code);
        }
        uv_mutex_unlock(&socket->uv_mutex_h);
        agent_request_finish(false, socket);

        socket->status = socket_init;
        return;
    }
	socket->status = socket_recv;

    //读取到数据
	if (socket->isbusy) {
        int code = uv_http_ok;
        if(socket->req->res == NULL) {
            socket->req->res = create_response(socket->req);
        }
		if(response_recive(socket->req->res, buf->base, nread, code)){
            //接收完成，执行下一个请求或者移动到空闲队列
            finish = true;
        }
	}
	uv_mutex_unlock(&socket->uv_mutex_h);

    if (finish) {
        agent_request_finish(true, socket);
        socket->status = socket_recv;
    }
}

/** 发送数据回调 */
static void write_cb(uv_write_t* req, int status) {
    socket_t* socket = (socket_t*)req->data;
	uv_mutex_lock(&socket->uv_mutex_h);
    if(status < 0) {
        fprintf(stderr, "write_cb error %s-%s\n", uv_err_name(status), uv_strerror(status)); 
        if(socket->req->req_cb) {
            socket->req->req_cb((request_t*)socket->req, uv_http_err_connect);
			uv_mutex_unlock(&socket->uv_mutex_h);
            return;
        }
    }

    socket->status = socket_send;
    if(socket->req->req_cb) {
        socket->req->req_cb((request_t*)socket->req, uv_http_ok);
		uv_mutex_unlock(&socket->uv_mutex_h);
        return;
    }
	uv_mutex_unlock(&socket->uv_mutex_h);
}

/** 发送数据 */
static void send_socket(socket_t* socket) {
	size_t body_num = 0;
    uv_buf_t *buf = NULL;
    int ret = 0;
	if (socket->req->body) {
		body_num = list_size(socket->req->body);
	}
    buf = (uv_buf_t *)malloc(sizeof(uv_buf_t)*(body_num+1));
    //http header
    generic_request_header(socket->req);
    *buf = uv_buf_init((char*)string_c_str(socket->req->str_header), string_size(socket->req->str_header));
    //http body
	if (socket->req->body) {
		list_iterator_t it_iter = list_begin(socket->req->body);
		list_iterator_t it_end = list_begin(socket->req->body);
		int i = 1;
		while (iterator_not_equal(it_iter, it_end)) {
			membuff_t mem = *(membuff_t*)iterator_get_pointer(it_iter);
			*(buf + i) = uv_buf_init((char*)mem.data, mem.len);
			it_iter = _list_iterator_next(it_iter);
		}
	}
    ret = uv_write(&socket->uv_write_h, (uv_stream_t*)&socket->uv_tcp_h, buf, body_num+1, write_cb);
    if (ret) {  
        fprintf(stderr, "uv_write error %s-%s\n", uv_err_name(ret), uv_strerror(ret)); 
        if(socket->req->req_cb) {
            socket->req->req_cb((request_t*)socket->req, uv_http_err_connect);
        }
    }  
}

/** tcp连接回调 */
static void connect_cb(uv_connect_t* conn, int status){
    socket_t* socket = (socket_t*)conn->data;
    uv_stream_t* handle = conn->handle;
    int ret = 0;
	uv_mutex_lock(&socket->uv_mutex_h);
    if(status < 0) {
        fprintf(stderr, "uv_connect_cb error %s-%s\n", uv_err_name(status), uv_strerror(status)); 
        if(socket->req->req_cb) {
            socket->req->req_cb((request_t*)socket->req, uv_http_err_connect);
        }
		uv_mutex_unlock(&socket->uv_mutex_h);
        return;
    }
    socket->status = socket_connected;

    //连接成功，开启数据接收
    handle->data = socket;
    ret = uv_read_start(handle, alloc_cb, read_cb);//客户端开始接收服务器的数据
    if (ret) {
        fprintf(stderr, "tcp receive failed:%s-%s", uv_err_name(ret), uv_strerror(ret)); 
        if(socket->req->req_cb) {
            socket->req->req_cb((request_t*)socket->req, uv_http_err_connect);
        }
		uv_mutex_unlock(&socket->uv_mutex_h);
        return;
    }

    //发送数据
    send_socket(socket);
	uv_mutex_unlock(&socket->uv_mutex_h);
}

/** 创建一个socket句柄 */
socket_t* create_socket(agent_t* agent) {
    socket_t* socket = (socket_t*)malloc(sizeof(socket_t));
    memset(socket, 0 , sizeof(socket_t));
    socket->agent = agent;
	socket->status = socket_uninit;
	uv_tcp_init(agent->handle->uv, &socket->uv_tcp_h);
	socket->status = socket_init;
	socket->uv_tcp_h.data = socket;
	socket->uv_connect_h.data = socket;
	socket->uv_write_h.data = socket;
	uv_mutex_init(&socket->uv_mutex_h);
    return socket;
}

/** 执行发送任务 */
void socket_run(socket_t* socket) {
	socket->isbusy = 1;
    if(socket->status == socket_uninit) {
        uv_tcp_init(socket->req->handle->uv, &socket->uv_tcp_h);
        socket->status = socket_init;
    }
    if(socket->status == socket_init){
        int ret = 0;
        socket->uv_connect_h.data = socket;
        ret = uv_tcp_connect(&socket->uv_connect_h, &socket->uv_tcp_h, socket->req->addr, connect_cb);
        if(ret < 0) {
            fprintf(stderr, "uv_tcp_connect error %s-%s\n", uv_err_name(ret),uv_strerror(ret)); 
            if(socket->req->req_cb) {
                socket->req->req_cb((request_t*)socket->req, uv_http_err_connect);
            }
        }
    } else {
        send_socket(socket);
    }
}

void destory_socket(socket_t* socket) {
    if (socket->status != socket_closed) {
        //外部直接调用销毁，先进行关闭tcp句柄，在回调中释放
        uv_close((uv_handle_t*)&socket->uv_tcp_h, close_cb);
    }
}