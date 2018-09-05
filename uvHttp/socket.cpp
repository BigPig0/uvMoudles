#include "public_type.h"
#include "typedef.h"

void close_cb(uv_handle_t* handle) {
    socket_t* socket = (socket_t*)handle->data;
    socket->status = socket_closed;
    destory_socket(socket);
}

void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    socket_t* socket = (socket_t*)handle->data;
    memset(socket->buff, 0, SOCKET_RECV_BUFF_LEN);
    *buf = uv_buf_init(socket->buff, SOCKET_RECV_BUFF_LEN);
}

extern void recive_response(request_p_t* req, char* data, int len);
extern void agent_free_socket(socket_t* socket);
void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    socket_t* socket = (socket_t*)stream->data;
    if (nread < 0) {
        if (nread == UV_EOF) {
            fprintf(stderr, "server close this socket");
        } else {
            fprintf(stderr, "read_cb error %s-%s\n", uv_err_name(nread), uv_strerror(nread)); 
        }
        uv_close((uv_handle_t*)stream, close_cb);
        return;
    }
	socket->status = socket_recv;

    //读取到数据
	if (socket->isbusy) {
		recive_response(socket->req, buf->base, buf->len);
		agent_free_socket(socket);
	} else {

	}
}

/** 发送数据回调 */
void write_cb(uv_write_t* req, int status) {
    socket_t* socket = (socket_t*)req->data;
    if(status < 0) {
        fprintf(stderr, "write_cb error %s-%s\n", uv_err_name(status), uv_strerror(status)); 
        if(socket->req->req_cb) {
            socket->req->req_cb(uv_http_err_connect, (request_t*)socket->req);
            return;
        }
    }

    socket->status = socket_send;
    if(socket->req->req_cb) {
        socket->req->req_cb(uv_http_ok, (request_t*)socket->req);
        return;
    }
}

/** 发送数据 */
void send_socket(socket_t* socket) {
    size_t body_num = list_size(socket->req->body);
    uv_buf_t *buf = (uv_buf_t *)malloc(sizeof(uv_buf_t)*(body_num+1));
    //http header
    *buf = uv_buf_init((char*)string_c_str(socket->req->str_header), string_size(socket->req->str_header));
    //http body
    list_iterator_t it_iter = list_begin(socket->req->body);
    list_iterator_t it_end = list_begin(socket->req->body);
    int i = 1;
    while (iterator_not_equal(it_iter, it_end))
    {
        membuff_t mem = *(membuff_t*)iterator_get_pointer(it_iter);
        *(buf+i) = uv_buf_init((char*)mem.data, mem.len);
        it_iter = _list_iterator_next(it_iter);
    }
    int ret = uv_write(&socket->uv_write_h, (uv_stream_t*)&socket->uv_tcp_h, buf, body_num+1, write_cb);
    if (ret) {  
        fprintf(stderr, "uv_write error %s-%s\n", uv_err_name(ret), uv_strerror(ret)); 
        if(socket->req->req_cb) {
            socket->req->req_cb(uv_http_err_connect, (request_t*)socket->req);
        }
    }  
}

/** tcp连接回调 */
void connect_cb(uv_connect_t* conn, int status){
    socket_t* socket = (socket_t*)conn->data;
    uv_stream_t* handle = conn->handle;
    if(status < 0) {
        fprintf(stderr, "uv_connect_cb error %s-%s\n", uv_err_name(status), uv_strerror(status)); 
        if(socket->req->req_cb) {
            socket->req->req_cb(uv_http_err_connect, (request_t*)socket->req);
        }
        return;
    }
    socket->status = socket_connected;

    //连接成功，开启数据接收
    handle->data = socket;
    int iret = uv_read_start(handle, alloc_cb, read_cb);//客户端开始接收服务器的数据
    if (iret) {
        fprintf(stderr, "tcp receive failed:%s-%s", uv_err_name(iret), uv_strerror(iret)); 
        if(socket->req->req_cb) {
            socket->req->req_cb(uv_http_err_connect, (request_t*)socket->req);
        }
        return;
    }

    //发送数据
    send_socket(socket);
}

/** 创建一个socket句柄 */
socket_t* create_socket(agent_t* agent) {
    socket_t* socket = (socket_t*)malloc(sizeof(socket_t));
    memset(socket, 0 , sizeof(socket_t));
    socket->agent = agent;
	uv_tcp_init(agent->handle->uv, &socket->uv_tcp_h);
	socket->uv_tcp_h.data = socket;
	socket->uv_connect_h.data = socket;
	socket->uv_write_h.data = socket;
	uv_mutex_init(&socket->uv_mutex_t);
    return socket;
}
/** 执行发送任务 */
void socket_run(socket_t* socket) {
    if(socket->status == socket_uninit) {
        uv_tcp_init(socket->req->handle->uv, &socket->uv_tcp_h);
        socket->status = socket_init;
    }
    if(socket->status == socket_init){
        socket->uv_connect_h.data = socket;
        int ret = uv_tcp_connect(&socket->uv_connect_h, &socket->uv_tcp_h, socket->req->addr, connect_cb);
        if(ret < 0) {
            fprintf(stderr, "uv_tcp_connect error %s-%s\n", uv_err_name(ret),uv_strerror(ret)); 
            if(socket->req->req_cb) {
                socket->req->req_cb(uv_http_err_connect, (request_t*)socket->req);
            }
        }
    } else {
        send_socket(socket);
    }
}

void destory_socket(socket_t* socket) {
    if (socket->status != socket_closed) {
        uv_close((uv_handle_t*)&socket->uv_tcp_h, close_cb);
    } else {
        agent_free_socket(socket);
    }
}