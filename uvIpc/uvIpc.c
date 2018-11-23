/*!
* \file uvIpc.c
* \date 2018/11/22 17:30
*
* \author wlla
* Contact: user@company.com
*
* \brief 
*
* TODO: long description
*
* \note
*/

#include "uv.h"
#include "cstl.h"
#include "uvIpc.h"
#include "netstream.h"

/** 服务端连接的客户 */
typedef struct _uv_ipc_clients_
{
    uv_ipc_handle_t         *ipc;
    uv_pipe_t               pipe;
    struct _uv_ipc_clients_ *pre;
    struct _uv_ipc_clients_ *next;
    char                    name[100];
}uv_ipc_clients_t;

typedef struct _uv_ipc_handle_ {
    //char*       ipc;        //pipe name
    int         is_svr;     //ture:服务端 false:客户端
    int         inner_uv;   //true:内部创建的 false:外部传入的
    uv_loop_t*  uv;
    uv_pipe_t   pipe;
    uv_ipc_clients_t *clients;  //服务端有用。
    uv_ipc_recv_cb cb;          //客户端回调函数
    char        name[100];      //客户端名称
}uv_ipc_handle_t;

typedef struct _uv_ipc_write_s_ {
    uv_ipc_handle_t *ipc;
    char* buff;  //接收时创建的缓存
    char* data;  //需要转发内容位置
    uint32_t num; //引用数
}uv_ipc_write_s_t;

typedef struct _uv_ipc_write_c_ {
    uv_ipc_handle_t *ipc;
    net_stream_maker_t* s;
}uv_ipc_write_c_t;

static void run_loop_thread(void* arg)
{
    uv_ipc_handle_t* h = (uv_ipc_handle_t*)arg;
    while (h->inner_uv) {
        uv_run(h->uv, UV_RUN_DEFAULT);
        Sleep(2000);
    }
    uv_loop_close(h->uv);
    free(h);
}

static void on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {//用于读数据时的缓冲区分配
    buf->base = (char*)malloc(suggested_size);
    buf->len = suggested_size;
}

//服务端回调

static void on_write_s(uv_write_t *req, int status) {
    uv_ipc_write_s_t* w = (uv_ipc_write_s_t*)req->data;
    if (status < 0) {
        fprintf(stderr, "Write error %s\n", uv_err_name(status));
    }
    w->num--;
    if(w->num == 0) {
        free(w->buff);
        free(w);
    }
    free(req);
}

static void on_read_s(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    uv_ipc_clients_t* c = (uv_ipc_clients_t*)client->data;
    uv_ipc_handle_t* ipc = (uv_ipc_handle_t*)c->ipc;
    char *recv_name, *next_name, *recvs, *data; 
    uint32_t recv_len, total_len;
    net_stream_parser_t *s;
    uv_ipc_write_s_t *w;

    if (nread < 0) {
        printf("Read error %s\n", uv_strerror(nread));
        uv_close((uv_handle_t*)client, NULL);
        if(c->pre) {
            c->pre->next = c->next;
            c->next->pre = c->pre;
        } else {
            c->ipc->clients = c->next;
            c->next->pre = NULL;
        }
        free(c);
        return;
    }

    //解析消息内容
    s         = create_net_stream_parser(buf->base, nread);
    recv_len  = net_stream_read_be32(s, 32);
    recvs     = net_stream_read_buff(s, recv_len);
    total_len = net_stream_read_be32(s, 32);
    data      = net_stream_read_buff(s, 0);
    if(c->name[0] == 0) {
        uint32_t send_len  = net_stream_read_be32(s, 32);
        char* send_name = net_stream_read_buff(s, send_len);
        strncpy(c->name, send_name, 100);
        c->name[99] = 0;
    }
    destory_net_stream_parser(s);

    w = (uv_ipc_write_s_t *)malloc(sizeof(uv_ipc_write_s_t));
    w->buff = buf->base;
    w->data = data;
    w->ipc = ipc;
    w->num = 0;

    //分割接受者名字
    recv_name = strtok_r(recvs, ",", &next_name);
    while (recv_name != NULL) { 
        uv_ipc_clients_t* tmp = ipc->clients;
        while(tmp != NULL) {
            if(!strncmp(recv_name, tmp->name, 100)) {
                uv_buf_t buff = uv_buf_init(data, total_len);
                uv_write_t *req = (uv_write_t *)malloc(sizeof(uv_write_t));
                w->num++;
                req->data = w;
                uv_write(req, (uv_stream_t*)&tmp->pipe, &buff, 1, on_write_s);
                //break; 允许同名
            }
        }
        recv_name = strtok_r(NULL, ",", &next_name); 
    } 
}

static void on_connection(uv_stream_t *server, int status) {
    //server's type is uv_pipe_t
    uv_ipc_handle_t* ipc = (uv_ipc_handle_t*)server->data;
    uv_ipc_clients_t* c;

    if (status < 0) {
        printf("new connect err:%s \n", uv_strerror(status));
        return;
    }

    c = (uv_ipc_clients_t*)malloc(sizeof(uv_ipc_clients_t));
    memset(c, 0, sizeof(uv_ipc_clients_t));
    c->ipc = ipc;
    uv_pipe_init(ipc->uv, &c->pipe, 0);
    if (uv_accept(server, (uv_stream_t*)&c->pipe) == 0) {//accept成功之后开始读
        c->next = ipc->clients;
        ipc->clients->pre = c;
        ipc->clients = c;
        c->pipe.data = c;
        uv_read_start((uv_stream_t*)&c->pipe, on_alloc, on_read_s);//开始从pipe读（在loop中注册读事件），读完回调
    } else {
        uv_close((uv_handle_t*)&c->pipe, NULL);//出错关闭
    }
}

// 户端回调

void on_write_c(uv_write_t *req, int status) {
    uv_ipc_write_c_t* w = (uv_ipc_write_c_t*)req->data;
    if (status < 0) {
        fprintf(stderr, "Write error %s\n", uv_err_name(status));
    }
    destory_net_stream_maker(w->s);
    free(w);
    free(req);
}

void on_read_c(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf){
    uv_ipc_handle_t* ipc = (uv_ipc_handle_t*)client->data;
    net_stream_parser_t *s;
    char *sender, *msg, *data;
    uint32_t sender_len, msg_len, data_len;
    if (nread < 0) {
        printf("read err:%s \n", uv_strerror(nread));
        return;
    }

    s          = create_net_stream_parser(buf->base, nread);
    sender_len = net_stream_read_be32(s, 32);
    sender     = net_stream_read_buff(s, sender_len);
    msg_len    = net_stream_read_be32(s, 32);
    msg        = net_stream_read_buff(s, msg_len);
    data_len   = net_stream_read_be32(s, 32);
    data       = net_stream_read_buff(s, data_len);

    sender[sender_len] = 0;
    msg[msg_len] = 0;

    if(ipc->cb)
        ipc->cb(sender, msg, data, data_len);

    free(buf->base);
}

void on_connect(uv_connect_t* req, int status){
    uv_ipc_handle_t* uvipc = (uv_ipc_handle_t*)req->data;
    if (status < 0) {
        printf("connect err:%s \n", uv_strerror(status));
        return;
    }

    uvipc = (uv_ipc_handle_t*)req->data;
    uv_read_start((uv_stream_t*)&uvipc->pipe, on_alloc, on_read_c);
}

//public api

int uv_ipc_server(uv_ipc_handle_t** h, char* ipc, void* uv) {
    uv_ipc_handle_t* uvipc = (uv_ipc_handle_t*)malloc(sizeof(uv_ipc_handle_t));
    //uvipc->ipc = ipc;
    uvipc->is_svr = 1;
    if(uv) {
        uvipc->uv = (uv_loop_t*)uv;
        uvipc->inner_uv = 0;
    } else {
        int ret;
        uv_thread_t tid;
        uvipc->uv = (uv_loop_t*)malloc(sizeof(uv_loop_t));
        ret = uv_loop_init(uvipc->uv);
        if(ret < 0) {
            printf("uv loop init failed: %s\n", uv_strerror(ret));
            free(uvipc->uv);
            free(uvipc);
            return ret;
        }
        uvipc->inner_uv = 1;
        uv_thread_create(&tid, run_loop_thread, uvipc);
        if(ret < 0) {
            printf("uv thread creat failed: %s\n", uv_strerror(ret));
            free(uvipc->uv);
            free(uvipc);
            return ret;
        }
    }

    uv_pipe_init(uvipc->uv, &uvipc->pipe, 0);
    uv_pipe_bind(&uvipc->pipe, ipc);
    uvipc->pipe.data = (void*)uvipc;
    uv_listen((uv_stream_t*)&uvipc->pipe, 128, on_connection);

    *h = uvipc;
    return 0;
}

int uv_ipc_client(uv_ipc_handle_t** h, char* ipc, void* uv, char* name, uv_ipc_recv_cb cb) {
    uv_ipc_handle_t* uvipc;
    int ret;
    uv_connect_t *conn;

    uvipc = (uv_ipc_handle_t*)malloc(sizeof(uv_ipc_handle_t));
    //uvipc->ipc = ipc;
    uvipc->is_svr = 0;
    uvipc->cb = cb;
    strncpy(uvipc->name, name, 100);
    uvipc->name[99] = 0;

    if(uv) {
        uvipc->uv = (uv_loop_t*)uv;
        uvipc->inner_uv = 0;
    } else {
        uv_thread_t tid;
        uvipc->uv = (uv_loop_t*)malloc(sizeof(uv_loop_t));
        ret = uv_loop_init(uvipc->uv);
        if(ret < 0) {
            printf("uv loop init failed: %s\n", uv_strerror(ret));
            free(uvipc->uv);
            free(uvipc);
            return ret;
        }
        uvipc->inner_uv = 1;
        uv_thread_create(&tid, run_loop_thread, uvipc);
        if(ret < 0) {
            printf("uv thread creat failed: %s\n", uv_strerror(ret));
            free(uvipc->uv);
            free(uvipc);
            return ret;
        }
    }

    ret = uv_pipe_init(uvipc->uv, &uvipc->pipe, 0);
    if(ret < 0) {
        printf("uv pipe init failed: %s\n", uv_strerror(ret));
        free(uvipc->uv);
        free(uvipc);
        return ret;
    }
    uvipc->pipe.data = (void*)uvipc;

    conn = (uv_connect_t *)malloc(sizeof(uv_connect_t));
    conn->data = uvipc;
    uv_pipe_connect(conn, &uvipc->pipe, ipc, on_connect); //连接pipe

    *h = uvipc;
    return 0;
}

int uv_ipc_send(uv_ipc_handle_t* h, char* names, char* msg, char* data, int len) {
    net_stream_maker_t* s;
    uv_ipc_write_c_t* w;
    uv_buf_t buff;
    uv_write_t *req;

    s = create_net_stream_maker();
    //receivers
    net_stream_append_be32(s, strlen(names));
    net_stream_append_string(s, names);
    //total len
    net_stream_append_be32(s, strlen(h->name) + strlen(msg) + len);
    //senders
    net_stream_append_be32(s, strlen(h->name));
    net_stream_append_string(s, h->name);
    //msg
    net_stream_append_be32(s, strlen(msg));
    net_stream_append_string(s, msg);
    //data
    net_stream_append_be32(s, len);
    net_stream_append_data(s, data, len);
    buff = uv_buf_init(get_net_stream_data(s), get_net_stream_len(s));

    w = (uv_ipc_write_c_t*)malloc(sizeof(uv_ipc_write_c_t));
    w->ipc = h;
    w->s = s;
    req = (uv_write_t *)malloc(sizeof(uv_write_t));
    req->data = w;
    uv_write(req, (uv_stream_t*)&h->pipe, &buff, 1, on_write_c);
    return 0;
}

void uv_ipc_close(uv_ipc_handle_t* h) {

}

char* uv_ipc_strerr(int status) {
    return "";
}