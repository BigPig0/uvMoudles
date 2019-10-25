#include "public.h"
#include "private.h"
#include "net.h"
#include "cstl_easy.h"
#include "dns.h"

/**
 * 异步数据
 */
typedef struct _net_socket_async_event_ {
    bool      write_data;
    bool      end;
    bool      pause;
    bool      resume;
}net_socket_async_event_t;

/**
 * Class: net.Server
 * This class is used to create a TCP or IPC server.
 */
typedef struct _net_server_ {
    uv_node_t      *handle;
    //options
    bool            allowHalfOpen;    //Indicates whether half-opened TCP connections are allowed. Default: false.
    bool            pauseOnConnect;   //Indicates whether the socket should be paused on incoming connections. Default: false.
    //listen_options;
    int             port;
    string_t        *host;
    int             backlog;     //Common parameter of server.listen() functions.
    bool            exclusive;   //Default: false
    bool            ipv6Only;    //For TCP servers, setting ipv6Only to true will disable dual-stack support, i.e., binding to host :: won't make 0.0.0.0 be bound. Default: false.
    //public
    bool            listening;        //<boolean> Indicates whether or not the server is listening for connections.
    int             maxConnections;   //Set this property to reject connections when the server's connection count gets high.
    //private
    uv_tcp_t        uv_tcp_h;
    on_server_event_connection    on_connection;
    on_server_event               on_close;
    on_event_error                on_error;
    on_server_event               on_listening;
    on_server_get_connections     on_get_connections;
}net_server_t;

/**
 * Class: net.Socket
 */
typedef struct _net_socket_ {
    uv_node_t      *handle;
    /**
     * net.Socket has the property that socket.write() always works. This is to help users get up and running quickly. The computer cannot always keep up with the amount of data that is written to a socket - the network connection simply might be too slow. Node.js will internally queue up the data written to a socket and send it out over the wire when it is possible. (Internally it is polling on the socket's file descriptor for being writable).
     * The consequence of this internal buffering is that memory may grow. This property shows the number of characters currently buffered to be written. (Number of characters is approximately equal to the number of bytes to be written, but the buffer may contain strings, and the strings are lazily encoded, so the exact number of bytes is not known.)
     * Users who experience large or growing bufferSize should attempt to "throttle" the data flows in their program with socket.pause() and socket.resume().
     */
    uint64_t    bufferSize; 
    uint64_t    bytesRead;    //The amount of received bytes.
    uint64_t    bytesWritten; //The amount of bytes sent.
    bool        destroyed;    //Indicates if the connection is destroyed or not. Once a connection is destroyed no further data can be transferred using it.
    bool        isKeepAlive;  //
    int         initialDelay; //<
    bool        noDelay;
    int         timeout;

    string_t                *localAddress;//The string representation of the local IP address the remote client is connecting on. For example, in a server listening on '0.0.0.0', if a client connects on '192.168.1.1', the value of socket.localAddress would be '192.168.1.1'.
    int                     localPort;    //The numeric representation of the local port. For example, 80 or 21.
    string_t                *remoteHost;
    string_t                *remoteAddress;//The string representation of the remote IP address. For example, '74.125.127.100' or '2001:4860:a005::68'. Value may be undefined if the socket is destroyed (for example, if the client disconnected).
    int                     remoteFamily; //The string representation of the remote IP family. 'IPv4' or 'IPv6'.
    int                     remotePort;   //The numeric representation of the remote port. For example, 80 or 21.
    //private
    uv_tcp_t                 uv_tcp_h;
    char                     readBuff[1024*1024];
    int                      readLen;
    //socket options
    bool                     allowHalfOpen;
    int                      fd;  //If specified, wrap around an existing socket with the given file descriptor, otherwise a new socket will be created.
    bool                     readable; // Allow reads on the socket when an fd is passed, otherwise ignored. Default: false.
    bool                     writable; // Allow writes on the socket when an fd is passed, otherwise ignored. Default: false.

    //server
    bool                     isServer;
    net_server_t             *server;
    //client
    bool                     connecting;   //If true, socket.connect(options[, connectListener]) was called and has not yet finished. Will be set to true before emitting 'connect' event and/or calling socket.connect(options[, connectListener])'s callback.
    bool                     pending;      //This is true if the socket is not connected yet, either because .connect() has not yet been called or because it is still in the process of connecting (see socket.connecting).
    //data
    list_t                   *write_list;   //向socket写入数据的缓存列表
    uv_mutex_t               wrlist_mutex;  //向socket写入数据的缓存列表的互斥锁
    uv_async_t               async_h;       //uv异步事件
    net_socket_async_event_t async_event;   //存在哪些异步事件类型

    on_socket_event_lookup   on_event_lookup;  //dns解析完成
    on_socket_event          on_event_connect; //客户端连接完成
    on_socket_event          on_event_ready;   //socket创建完成
    on_socket_event_data     on_event_data;    //读取到一段数据
    on_socket_event_close    on_event_close;   //socket关闭
    on_socket_event          on_event_drain;   //写数据变空
    on_socket_event          on_event_end;     //对端发送了FIN，读取到EOF
    on_socket_event_error    on_event_error;   //发生错误
    on_socket_event          on_event_timeout; //超时
}net_socket_t;

//////////////////////////////////////////////////////////////////////////

void net_create_server_async(void* p){
    net_server_t* net = (net_server_t*)p;
    uv_tcp_init(net->handle->uv, &net->uv_tcp_h);
}

net_server_t* net_create_server(uv_node_t* h, char** options /*= NULL*/, on_server_event_connection connectionListener /*= NULL*/) {
    uv_thread_t tid = uv_thread_self();
    SAFE_MALLOC(net_server_t, ret);
    ret->handle = h;
    ret->uv_tcp_h.data = ret;
    ret->backlog = 511;
    if (options) {
        int i = 0;
        char *key = options[0], *value = options[1];
        for(;key&&value;key = options[i], value = options[i+1]){
            if(!strcasecmp(key,"allowHalfOpen")){
                if(!strcasecmp(value,"true"))
                    ret->allowHalfOpen = true;
            } else if(!strcasecmp(key,"pauseOnConnect")){
                if(!strcasecmp(value,"true"))
                    ret->pauseOnConnect = true;
            }
            i += 2;
        }
    }
    if (connectionListener)
        ret->on_connection = connectionListener;

    if(tid == h->loop_tid){
        uv_tcp_init(ret->handle->uv, &ret->uv_tcp_h);
    } else {
        send_async_event(h, ASYNC_EVENT_TCP_SERVER_INIT, ret);
    }

    return ret;
}

void net_server_on_close(net_server_t* svr, on_server_event cb) {
    svr->on_close = cb;
}

void net_server_on_connection(net_server_t* svr, on_server_event_connection cb) {
    svr->on_connection = cb;
}

void net_server_on_error(net_server_t* svr, on_event_error cb) {
    svr->on_error = cb;
}

void net_server_on_listening(net_server_t* svr, on_server_event cb) {
    svr->on_listening = cb;
}

int net_server_address(net_server_t* svr, net_address_t* address) {
    int ret = 0;
    struct sockaddr name;
    struct sockaddr_in* sin = (struct sockaddr_in*)&name;
    struct sockaddr_in6* sin6 = (struct sockaddr_in6*)&name;
    int namelen;
    ret = uv_tcp_getsockname(&svr->uv_tcp_h, &name, &namelen);
    if(ret < 0) {
        printf("uv_tcp_getsockname error: %s\n", uv_strerror(ret));
        return 0;
    }
    if(name.sa_family == AF_INET) {
        address->family = 4;
        uv_ip4_name(sin, address->address, 46);
        address->port = sin->sin_port;
        return 4;
    } else if(name.sa_family == AF_INET6) {
        address->family = 6;
        uv_ip6_name(sin6, address->address, 46);
        address->port = sin6->sin6_port;
        return 6;
    }
    
    return 0;
}

void net_server_close(net_server_t* svr, on_server_event cb /*= NULL*/) {
    if(cb) svr->on_close = cb;
}

void net_server_get_connections(net_server_t* svr, on_server_get_connections cb /*= NULL*/) {
    if(cb) svr->on_get_connections = cb;
}

void net_server_listen_handle(net_server_t* svr, void* handle, int backlog /*= 0*/, on_server_event callback /*= NULL*/) {

}

static void on_uv_close(uv_handle_t* handle) {
    net_socket_t *skt = (net_socket_t*)handle->data;
    printf("close client %s  %d\n", skt->remoteAddress, skt->remotePort);
    if(skt->on_event_close) skt->on_event_close(skt, false);
}

static void on_uv_shutdown(uv_shutdown_t* req, int status) {
    net_socket_t *skt = (net_socket_t*)req->data;
    free(req);
    if(!skt->allowHalfOpen){
        uv_close((uv_handle_t*)&skt->uv_tcp_h, on_uv_close);
    }
}

static void on_uv_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
    net_socket_t *skt = (net_socket_t*)handle->data;
    *buf = uv_buf_init(skt->readBuff, 1024*1024);
}

static void on_uv_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    net_socket_t *skt = (net_socket_t*)stream->data;
    if(nread < 0) {
        if(nread == UV__ECONNRESET || nread == UV_EOF) {
            //对端发送了FIN
            if(skt->on_event_end) skt->on_event_end(skt);
            if(!skt->allowHalfOpen){
                uv_close((uv_handle_t*)&skt->uv_tcp_h, on_uv_close);
            }
        } else {
            uv_shutdown_t* req = (uv_shutdown_t*)malloc(sizeof(uv_shutdown_t));
            req->data = skt;
            printf("Read error %s\n", uv_strerror(nread));
            if(skt->on_event_error) skt->on_event_error(skt, nread);
            uv_shutdown(req, stream, on_uv_shutdown);
        }
        return;
    }
    skt->bytesRead += nread;
    if (skt->on_event_data) skt->on_event_data(skt, buf->base, nread);
}

static void on_connection(uv_stream_t* server, int status) {
    net_server_t* svr = (net_server_t*)server->data;
    net_socket_t* client;
    struct sockaddr sockname, peername;
    int namelen;
    int ret;
    if (status != 0) {
        printf("Connect error %s\n");
        if(svr->on_connection) {
            svr->on_connection(svr, status, NULL);
        }
        return;
    }

    client = net_create_socket(svr->handle, NULL);
    client->isServer = true;
    client->server = svr;
    client->allowHalfOpen = svr->allowHalfOpen;

    ret = uv_accept(server, (uv_stream_t*)(&client->uv_tcp_h));

    memset(&sockname, -1, sizeof sockname);
    namelen = sizeof(struct sockaddr);
    ret = uv_tcp_getsockname(&client->uv_tcp_h, &sockname, &namelen);
    namelen = sizeof(struct sockaddr);
    ret = uv_tcp_getpeername(&client->uv_tcp_h, &peername, &namelen);
    if(peername.sa_family == AF_INET) {
        struct sockaddr_in* sin = (struct sockaddr_in*)&peername;
        client->remoteFamily = 4;
        uv_ip4_name(sin, client->remoteAddress, 46);
        client->remotePort = sin->sin_port;
        sin = (struct sockaddr_in*)&sockname;
        uv_ip4_name(sin, client->localAddress, 46);
        client->localPort = sin->sin_port;
    } else if(peername.sa_family == AF_INET6) {
        struct sockaddr_in6* sin6 = (struct sockaddr_in6*)&peername;
        client->remoteFamily = 6;
        uv_ip6_name(sin6, client->remoteAddress, 46);
        client->remotePort = sin6->sin6_port;
        sin6 = (struct sockaddr_in6*)&sockname;
        uv_ip6_name(sin6, client->localAddress, 46);
        client->localPort = sin6->sin6_port;
    }

    if(svr->on_connection) svr->on_connection(svr, 0, client);

    if(!svr->pauseOnConnect){
        ret = uv_read_start((uv_stream_t*)&client->uv_tcp_h, on_uv_alloc, on_uv_read);
        if(ret < 0) {
            printf("error\n");
        }
    }
}

static void on_dns_lookup(int err, char *address, int family, void *user) {
    net_server_t* svr = (net_server_t*)user;
    int ret;
    if(!err && svr->on_listening) {
        svr->on_listening(svr, err);
        return;
    }
    if(family == 4) {
        struct sockaddr_in addr;
        ret = uv_ip4_addr(address, svr->port, &addr);
        uv_tcp_bind(&svr->uv_tcp_h, (struct sockaddr*)&addr, 0);
        uv_listen((uv_stream_t*)&svr->uv_tcp_h, svr->backlog, on_connection);
        svr->listening = true;
        svr->on_listening(svr, 0);
    } else if(family == 6) {
        struct sockaddr_in6 addr6;
        ret = uv_ip6_addr(address, svr->port, &addr6);
        uv_tcp_bind(&svr->uv_tcp_h, (struct sockaddr*)&addr6, 0);
        uv_listen((uv_stream_t*)&svr->uv_tcp_h, svr->backlog, on_connection);
        svr->listening = true;
        svr->on_listening(svr, 0);
    } else {
        svr->on_listening(svr, -1);
    }
}

static void _net_server_listen(net_server_t* svr) {
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;
    int ret;
    if(svr->port != 0) {
        if(svr->host) {
            dns_lookup(svr->handle, (char*)string_c_str(svr->host), on_dns_lookup, svr);
        } else if(svr->ipv6Only) {
            ret = uv_ip6_addr("::", svr->port, &addr6);
            ret = uv_tcp_bind(&svr->uv_tcp_h, (struct sockaddr*)&addr, 0);
            ret = uv_listen((uv_stream_t*)&svr->uv_tcp_h, svr->backlog, on_connection);
            svr->listening = true;
            svr->on_listening(svr, 0);
        } else {
            ret = uv_ip4_addr("0.0.0.0", svr->port, &addr);
            ret = uv_tcp_bind(&svr->uv_tcp_h, (struct sockaddr*)&addr, 0);
            ret = uv_listen((uv_stream_t*)&svr->uv_tcp_h, svr->backlog, on_connection);
            svr->listening = true;
            svr->on_listening(svr, 0);
        }

    } else {
        abort();
    }
}

void net_server_listen_options(net_server_t* svr, char** options, on_server_event callback /*= NULL*/) {
    if(options){
        int i = 0;
        char *key = options[0], *value = options[1];
        for(;key&&value;key = options[i], value = options[i++]){
            if(!strcasecmp(key, "port")) {
                svr->port = atoi(value);
            } else if(!strcasecmp(key, "host")) {
                svr->host = create_string();
                string_init_cstr(svr->host, value);
            }else if(!strcasecmp(key, "backlog")) {
                svr->backlog = atoi(value);
            }else if(!strcasecmp(key, "exclusive")) {
                if(!strcasecmp(value,"true"))
                    svr->exclusive = true;
            }else if(!strcasecmp(key, "ipv6Only")) {
                if(!strcasecmp(value, "true"))
                    svr->ipv6Only = true;
            }
            i += 2;
        }
    }
   
    if(callback)
        svr->on_listening = callback;

   _net_server_listen(svr);
}

void net_server_listen_port(net_server_t* svr, int port, char* host, int backlog /*= 0*/, on_server_event callback /*= NULL*/) {
    svr->port = port;
    if(host){
        svr->host = create_string();
        string_init_cstr(svr->host, host);
    }
    if(backlog > 0) {
        svr->backlog = backlog;
    }
    if(callback)
        svr->on_listening = callback;

    _net_server_listen(svr);
}

//////////////////////////////////////////////////////////////////////////

net_socket_connect_options_t* net_socket_create_connect_options() {
    SAFE_MALLOC(net_socket_connect_options_t, ret);
    ret->host = "localhost";
    ret->family = 4;
    return ret;
}

/** 数据发送完成 */
static void on_uv_write(uv_write_t* req, int status) {
    net_socket_t* skt = (net_socket_t*)req->data;
    printf("write finish %d\n", status);
}

/** 异步发送数据 */
static void on_uv_async_event(uv_async_t* handle) {
    net_socket_t* skt = (net_socket_t*)handle->data;
    if(skt->async_event.write_data){
        skt->async_event.write_data = false;
        uv_mutex_lock(&skt->wrlist_mutex);
        {
            int num = list_size(skt->write_list);
            SAFE_MALLOC(uv_write_t, req);
            int i=0;
            uv_buf_t *bufs = (uv_buf_t*)malloc(sizeof(uv_buf_t)*num);
            LIST_FOR_BEGIN(skt->write_list,uv_buf_t*,wrbuff) {
                bufs[i].base = wrbuff->base;
                bufs[i].len = wrbuff->len;
                free(wrbuff);
                i++;
            } LIST_FOR_END;
            req->data = skt;
            uv_write(req, (uv_stream_t*)&skt->uv_tcp_h, bufs, num, on_uv_write);
            list_clear(skt->write_list);
            free(bufs);
        }
        uv_mutex_unlock(&skt->wrlist_mutex);
    } 
    if(skt->async_event.end) {
        uv_shutdown_t* req;
        skt->async_event.end = false;
        req = (uv_shutdown_t*)malloc(sizeof(uv_shutdown_t));
        req->data = skt;
        uv_shutdown(req, (uv_stream_t*)&skt->uv_tcp_h, on_uv_shutdown);
    } 
    if(skt->async_event.pause) {
        skt->async_event.pause = false;
        uv_read_stop((uv_stream_t*)&skt->uv_tcp_h);
    } 
    if(skt->async_event.resume) {
        skt->async_event.resume = false;
        uv_read_start((uv_stream_t*)&skt->uv_tcp_h, on_uv_alloc, on_uv_read);
    }
}

net_socket_t* net_create_socket(uv_node_t* h, char **options /*= NULL*/) {
    SAFE_MALLOC(net_socket_t, ret);
    ret->handle = h;
    uv_tcp_init(h->uv, &ret->uv_tcp_h);
    ret->uv_tcp_h.data = ret;
    uv_mutex_init(&ret->wrlist_mutex);
    uv_async_init(h->uv, &ret->async_h, on_uv_async_event);
    ret->async_h.data = ret;
    if(options){
        int i = 0;
        char *key = options[0], *value = options[1];
        for(;key&&value;key = options[i], value = options[i+1]){
            if(!strcasecmp(key,"allowHalfOpen")){
                if(!strcasecmp(value,"true"))
                    ret->allowHalfOpen = true;
            } else if(!strcasecmp(key,"fd")){
                ret->fd = atoi(value);
                uv_tcp_open(&ret->uv_tcp_h, ret->fd);
            }
            i += 2;
        }
    }
    return ret;
}

void net_socket_on_close(net_socket_t* skt, on_socket_event_close cb) {
    skt->on_event_close = cb;
}

void net_socket_on_connect(net_socket_t* skt, on_socket_event cb) {
    skt->on_event_connect = cb;
}

void net_socket_on_data(net_socket_t* skt, on_socket_event_data cb) {
    skt->on_event_data = cb;
}

void net_socket_on_drain(net_socket_t* skt, on_socket_event cb) {
    skt->on_event_drain = cb;
}

void net_socket_on_end(net_socket_t* skt, on_socket_event cb) {
    skt->on_event_end = cb;
}

void net_socket_on_error(net_socket_t* skt, on_socket_event_error cb) {
    skt->on_event_error = cb;
}

void net_socket_on_lookup(net_socket_t* skt, on_socket_event_lookup cb) {
    skt->on_event_lookup = cb;
}

void net_socket_on_ready(net_socket_t* skt, on_socket_event cb) {
    skt->on_event_ready = cb;
}

void net_socket_on_timeout(net_socket_t* skt, on_socket_event cb) {
    skt->on_event_timeout = cb;
}

net_address_t net_socket_address(net_socket_t* skt) {
    struct sockaddr sockname;
    int namelen;
    int ret;
    net_address_t address;

    memset(&sockname, -1, sizeof sockname);
    namelen = sizeof(struct sockaddr);
    ret = uv_tcp_getsockname(&skt->uv_tcp_h, &sockname, &namelen);
    if(sockname.sa_family == AF_INET) {
        struct sockaddr_in* sin = (struct sockaddr_in*)&sockname;
        address.family = 4;
        uv_ip4_name(sin, address.address, 46);
        address.port = sin->sin_port;
    } else if(sockname.sa_family == AF_INET6) {
        struct sockaddr_in6* sin6 = (struct sockaddr_in6*)&sockname;
        address.family = 6;
        uv_ip6_name(sin6, address.address, 46);
        address.port = sin6->sin6_port;
    }

    return address;
}

static void on_uv_connect(uv_connect_t* req, int status){
    net_socket_t* skt = (net_socket_t*)req->data;
    free(req);
    if(skt->on_event_connect) skt->on_event_connect(skt);
    skt->connecting = false;
    skt->pending = false;
    uv_read_start((uv_stream_t*)&skt->uv_tcp_h, on_uv_alloc, on_uv_read);
}

static void on_socket_dns_lookup(int err, char *address, int family, void* user) {
    net_socket_t* skt = (net_socket_t*)user;
    if(err) {
        if(skt->on_event_lookup) skt->on_event_lookup(skt, err, address, family, 0);
        return;
    } else if(family == 6) {
        struct sockaddr_in6 addr6;
        int ret = uv_ip6_addr(address, skt->remotePort, &addr6);
        uv_connect_t *req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
        req->data = skt;

        if(skt->on_event_lookup) skt->on_event_lookup(skt, 0, address, family, (char*)string_c_str(skt->remoteAddress));
        
        ret = uv_tcp_connect(req, &skt->uv_tcp_h, (struct sockaddr*)&addr6, on_uv_connect);

        if(skt->on_event_ready) skt->on_event_ready(skt);
    } else if(family == 4){
        struct sockaddr_in addr;
        int ret = uv_ip4_addr(address, skt->remotePort, &addr);
        uv_connect_t *req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
        req->data = skt;

        if(skt->on_event_lookup) skt->on_event_lookup(skt, 0, address, family, skt->remoteAddress);
        
        ret = uv_tcp_connect(req, &skt->uv_tcp_h, (struct sockaddr*)&addr, on_uv_connect);

        if(skt->on_event_ready) skt->on_event_ready(skt);
    } else {
        if(skt->on_event_lookup) skt->on_event_lookup(skt, -1, address, family, 0);
    }
}

void net_socket_connect_options(net_socket_t* skt, char **options, on_socket_event connectListener /*= NULL*/) {
    int ret;
    dns_resolver_t* dns;

    /** 事件绑定 */
    if(connectListener) 
        skt->on_event_connect = connectListener;

    /** TCP */
    if(options) {
        int i = 0;
        char *key = options[0], *value = options[1];
        for(;key&&value;key = options[i], value = options[i+1]){
            if(!strcasecmp(key,"port")){
                skt->remotePort = atoi(value);
            } else if(!strcasecmp(key,"host")){
                skt->remoteHost = create_string();
                string_init_cstr(skt->remoteHost, value);
            } else if(!strcasecmp(key,"localPort")){
                skt->localPort = atoi(value);
            }  else if(!strcasecmp(key,"localAddress")){
                skt->localAddress = create_string();
                string_init_cstr(skt->localAddress, value);
            } else if(!strcasecmp(key,"family")){
               if(6 == atoi(value))
                   skt->remoteFamily = 6;
            } else if(!strcasecmp(key,"hints")){
            } 
            i += 2;
        }
    }
    if (skt->remotePort) {
        if(skt->localPort) {
            if(skt->remoteFamily == 6) {
                struct sockaddr_in6 addr;
                ret = uv_ip6_addr(string_c_str(skt->localAddress), skt->localPort, &addr);
                ret = uv_tcp_bind(&skt->uv_tcp_h, (struct sockaddr*)&addr, 0);
            } else {
                struct sockaddr_in addr;
                ret = uv_ip4_addr(string_c_str(skt->localAddress), skt->localPort, &addr);
                ret = uv_tcp_bind(&skt->uv_tcp_h, (struct sockaddr*)&addr, 0);
            }
        }

        dns = dns_create_resolver(skt->handle);
        dns_set_data(dns, skt);
        dns_lookup(skt->handle, (char*)string_c_str(skt->remoteHost), on_socket_dns_lookup, skt);
    }
}

void net_socket_connect_port(net_socket_t* skt, int port, char *host /*= NULL*/, on_socket_event connectListener /*= NULL*/) {
    net_socket_connect_options_t *options = net_socket_create_connect_options();
    options->port = port;
    if(host) options->host = host;
    net_socket_connect_options(skt, options, connectListener);
}

void net_socket_destory(net_socket_t* skt, int err /*= 0*/) {

}

void net_socket_end(net_socket_t* skt, char* data /*= NULL*/, int len /*= 0*/, on_socket_event cb /*= NULL*/) {
    //if(cb) skt->on_event_end = cb;
    if(data != NULL && len != 0) {
        uv_buf_t* wrdata = (uv_buf_t*)malloc(sizeof(uv_buf_t));
        wrdata->base = data;
        wrdata->len = len;
        uv_mutex_lock(&skt->wrlist_mutex);
        if(!skt->write_list) {
            skt->write_list = create_list(void*);
            list_init(skt->write_list);
        }
        list_push_back(skt->write_list, (void*)wrdata);
        uv_mutex_unlock(&skt->wrlist_mutex);

        skt->async_event.write_data = true;
        uv_async_send(&skt->async_h);
    }
    skt->async_event.end = true;
    uv_async_send(&skt->async_h);
}

void net_socket_pause(net_socket_t* skt) {
    skt->async_event.pause = true;
    uv_async_send(&skt->async_h);
}

void net_socket_resume(net_socket_t* skt) {
    skt->async_event.resume = true;
    uv_async_send(&skt->async_h);
}

void net_socket_set_keep_alive(net_socket_t* skt, bool enable /*= false*/, int initialDelay /*= 0*/) {
    uv_tcp_keepalive(&skt->uv_tcp_h, enable, initialDelay);
}

void net_socket_set_no_delay(net_socket_t* skt, bool noDelay /*= true*/) {
    uv_tcp_nodelay(&skt->uv_tcp_h, noDelay);
}

void net_socket_set_timeout(net_socket_t* skt, int timeout, on_socket_event cb /*= NULL*/) {
    if(cb) skt->on_event_timeout = cb;
}

bool net_socket_write(net_socket_t* skt, char *data, int len, on_socket_event cb /*= NULL*/) {
    uv_buf_t* wrdata = (uv_buf_t*)malloc(sizeof(uv_buf_t));
    wrdata->base = data;
    wrdata->len = len;
    uv_mutex_lock(&skt->wrlist_mutex);
    if(!skt->write_list) {
        skt->write_list = create_list(void*);
        list_init(skt->write_list);
    }
    list_push_back(skt->write_list, (void*)wrdata);
    uv_mutex_unlock(&skt->wrlist_mutex);

    skt->async_event.write_data = true;
    uv_async_send(&skt->async_h);
    return true;
}

/**
 * ------------------------------------------------------------------------------------------------
 */

net_socket_t* net_connect_options(uv_node_t* h, char **options, on_socket_event cb /*= NULL*/) {
    net_socket_t* ret = net_create_socket(h, options);

    net_socket_connect_options(ret, options, cb);
    return ret;
}

net_socket_t* net_connect_port(uv_node_t* h, int port, char *host, on_socket_event cb /*= NULL*/) {
    net_socket_t* ret = net_create_socket(h, NULL);

    net_socket_connect_port(ret, port, host, cb);
    return ret;
}

int net_is_ip(char* input) {
    if(net_is_ipv4(input))
        return 4;
    if(net_is_ipv6(input))
        return 6;
    return 0;
}

bool net_is_ipv4(char* input) {
    struct sockaddr_in addr;

    if (!input) return false;
    if (strchr(input, '.')) return false;
    if (uv_inet_pton(AF_INET, input, &addr.sin_addr) != 0) return false;

    return true;
}

bool net_is_ipv6(char* input) {
    struct sockaddr_in6 addr;

    if (!input) return false;
    if (strchr(input, '.')) return false;
    if (uv_inet_pton(AF_INET6, input, &addr.sin6_addr) != 0) return false;

    return true;
}