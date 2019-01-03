#include "public_def.h"
#include "private_def.h"
#include "net.h"
#include "cstl_easy.h"
#include "dns.h"

/**
 * Class: net.Server
 * This class is used to create a TCP or IPC server.
 */
typedef struct _net_server_ {
    bool            allowHalfOpen;    //Indicates whether half-opened TCP connections are allowed. Default: false.
    bool            pauseOnConnect;   //Indicates whether the socket should be paused on incoming connections. Default: false.
    bool            listening;        //<boolean> Indicates whether or not the server is listening for connections.
    int             maxConnections;   //Set this property to reject connections when the server's connection count gets high.
    net_server_listen_options_t* listen_options;
    //private
    http_t*         handle;
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

    char        localAddress[46];//The string representation of the local IP address the remote client is connecting on. For example, in a server listening on '0.0.0.0', if a client connects on '192.168.1.1', the value of socket.localAddress would be '192.168.1.1'.
    int         localPort;    //The numeric representation of the local port. For example, 80 or 21.
    char        remoteAddress[46];//The string representation of the remote IP address. For example, '74.125.127.100' or '2001:4860:a005::68'. Value may be undefined if the socket is destroyed (for example, if the client disconnected).
    int         remoteFamily; //The string representation of the remote IP family. 'IPv4' or 'IPv6'.
    int         remotePort;   //The numeric representation of the remote port. For example, 80 or 21.
    //private
    http_t*                  handle;
    uv_tcp_t                 uv_tcp_h;
    char                     readBuff[1024*1024];
    int                      readLen;
    bool                     allowHalfOpen;
    bool                     readable;
    bool                     writable;
    //server
    bool                     isServer;
    net_server_t             *server;
    //client
    bool                     connecting;   //If true, socket.connect(options[, connectListener]) was called and has not yet finished. Will be set to true before emitting 'connect' event and/or calling socket.connect(options[, connectListener])'s callback.
    bool                     pending;      //This is true if the socket is not connected yet, either because .connect() has not yet been called or because it is still in the process of connecting (see socket.connecting).
    list_t                   *write_list;
    uv_mutex_t               wrlist_mutex;
    on_socket_event_lookup   on_event_lookup;
    on_socket_event          on_event_connect;
    on_socket_event          on_event_ready;
    on_socket_event_data     on_event_data;
    on_socket_event_close    on_event_close;
    on_socket_event          on_event_drain;
    on_socket_event          on_event_end;
    on_socket_event_error    on_event_error;
    on_socket_event          on_event_timeout;
}net_socket_t;



net_server_listen_options_t* net_server_create_listen_options() {
    SAFE_MALLOC(net_server_listen_options_t, ret);
    return ret;
}

net_server_t* net_create_server(http_t* h, net_server_options_t* options /*= NULL*/, on_server_event_connection connectionListener /*= NULL*/) {
    SAFE_MALLOC(net_server_t, ret);
    ret->handle = h;
    ret->uv_tcp_h.data = ret;
    if (options) {
        ret->allowHalfOpen = options->allowHalfOpen;
        ret->pauseOnConnect = options->pauseOnConnect;
    }
    if (connectionListener)
        ret->on_connection = connectionListener;

    uv_tcp_init(ret->handle->uv, &ret->uv_tcp_h);

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
    uv_close((uv_handle_t*)&skt->uv_tcp_h, on_uv_close);
}

static void on_uv_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
    net_socket_t *skt = (net_socket_t*)handle->data;
    *buf = uv_buf_init(skt->readBuff, 1024*1024);
}

static void on_uv_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    net_socket_t *skt = (net_socket_t*)stream->data;
    if(nread < 0) {
        if(nread == UV_EOF) {
            if(skt->on_event_end) skt->on_event_end(skt);
            uv_close((uv_handle_t*)&skt->uv_tcp_h, on_uv_close);
        } else {
            uv_shutdown_t* req = (uv_shutdown_t*)malloc(sizeof(uv_shutdown_t));
            req->data = skt;
            printf("Read error %s\n", uv_strerror(nread));
            if(skt->on_event_error) skt->on_event_error(skt, nread);
            uv_shutdown(req, stream, on_uv_shutdown);
        }
    }
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

    ret = uv_read_start((uv_stream_t*)client, on_uv_alloc, on_uv_read);
}

static void on_dns_lookup(dns_resolver_t* res, int err, char *address, int family) {
    net_server_t* svr = (net_server_t*)dns_get_data(res);
    int ret;
    if(!err && svr->on_listening) {
        svr->on_listening(svr, err);
        return;
    }
    if(family == 4) {
        struct sockaddr_in addr;
        ret = uv_ip4_addr(address, svr->listen_options->port, &addr);
        uv_tcp_bind(&svr->uv_tcp_h, (struct sockaddr*)&addr, 0);
        uv_listen((uv_stream_t*)&svr->uv_tcp_h, svr->listen_options->backlog, on_connection);
        svr->on_listening(svr, 0);
    } else if(family == 6) {
        struct sockaddr_in6 addr6;
        ret = uv_ip6_addr(address, svr->listen_options->port, &addr6);
        uv_tcp_bind(&svr->uv_tcp_h, (struct sockaddr*)&addr6, 0);
        uv_listen((uv_stream_t*)&svr->uv_tcp_h, svr->listen_options->backlog, on_connection);
        svr->on_listening(svr, 0);
    } else {
        svr->on_listening(svr, -1);
    }
}

void net_server_listen_options(net_server_t* svr, net_server_listen_options_t* options, on_server_event callback /*= NULL*/) {
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;
    int ret;
    svr->listen_options = options;
    if(callback) {
        svr->on_listening = callback;
    }
    if(options->port != 0) {
        if(options->host) {
            dns_resolver_t* dns = dns_create_resolver(svr->handle);
            dns_set_data(dns, svr);
            dns_lookup(dns, options->host, on_dns_lookup);
        } else if(options->ipv6Only) {
            ret = uv_ip6_addr("::", options->port, &addr6);
            uv_tcp_bind(&svr->uv_tcp_h, (struct sockaddr*)&addr, 0);
            uv_listen((uv_stream_t*)&svr->uv_tcp_h, options->backlog, on_connection);
        } else {
            ret = uv_ip4_addr("0.0.0.0", options->port, &addr);
            uv_tcp_bind(&svr->uv_tcp_h, (struct sockaddr*)&addr, 0);
            uv_listen((uv_stream_t*)&svr->uv_tcp_h, options->backlog, on_connection);
        }
        
    } else if(options->path) {

    } else {
        abort();
    }
}

void net_server_listen_path(net_server_t* svr, char* path, int backlog /*= 0*/, on_server_event callback /*= NULL*/) {
    net_server_listen_options_t* options = net_server_create_listen_options();
    options->path = path;
    options->backlog = backlog;
    net_server_listen_options(svr, options, callback);
}

void net_server_listen_port(net_server_t* svr, int port, char* host, int backlog /*= 0*/, on_server_event callback /*= NULL*/) {
    net_server_listen_options_t* options = net_server_create_listen_options();
    options->port = port;
    options->host = host;
    options->backlog = backlog;
    net_server_listen_options(svr, options, callback);
}



net_socket_connect_options_t* net_socket_create_connect_options() {
    SAFE_MALLOC(net_socket_connect_options_t, ret);
    ret->host = "localhost";
    ret->family = 4;
    return ret;
}

net_socket_t* net_create_socket(http_t* h, net_socket_options_t *option /*= NULL*/) {
    SAFE_MALLOC(net_socket_t, ret);
    ret->handle = h;
    uv_tcp_init(h->uv, &ret->uv_tcp_h);
    ret->uv_tcp_h.data = ret;
    if(option){
        ret->allowHalfOpen = option->allowHalfOpen;
        ret->readable = option->readable;
        ret->writable = option->writable;
        if(option->fd) {
            uv_tcp_open(&ret->uv_tcp_h, option->fd);
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
}

static void on_socket_dns_lookup(dns_resolver_t* res, int err, char *address, int family) {
    net_socket_t* skt = (net_socket_t*)dns_get_data(res);
    if(!err) {
        if(skt->on_event_lookup) skt->on_event_lookup(skt, err, address, family, 0);
        return;
    } else if(family == 6) {
        struct sockaddr_in6 addr6;
        int ret = uv_ip6_addr(address, skt->remotePort, &addr6);
        uv_connect_t *req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
        req->data = skt;

        if(skt->on_event_lookup) skt->on_event_lookup(skt, 0, address, family, skt->remoteAddress);
        
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

void net_socket_connect_options(net_socket_t* skt, net_socket_connect_options_t *options, on_socket_event connectListener /*= NULL*/) {
    int ret;
    dns_resolver_t* dns;

    /** ÊÂ¼þ°ó¶¨ */
    if(connectListener) 
        skt->on_event_connect = connectListener;

    /** TCP */
    if (options->port) {
        skt->remotePort = options->port;
        if(options->localPort) {
            skt->localPort = options->localPort;
            if(options->family == 6) {
                struct sockaddr_in addr;
                ret = uv_ip4_addr(options->localAddress, options->localPort, &addr);
                ret = uv_tcp_bind(&skt->uv_tcp_h, (struct sockaddr*)&addr, 0);
            } else {
                struct sockaddr_in addr;
                ret = uv_ip4_addr(options->localAddress, options->localPort, &addr);
                ret = uv_tcp_bind(&skt->uv_tcp_h, (struct sockaddr*)&addr, 0);
            }
        }

        dns = dns_create_resolver(skt->handle);
        dns_set_data(dns, skt);
        dns_lookup(dns, options->host, on_socket_dns_lookup);
    }
    /** IPC */
    else if(options->path) {

    }
}

void net_socket_connect_path(net_socket_t* skt, char* path, on_socket_event connectListener /*= NULL*/) {
    net_socket_connect_options_t *options = net_socket_create_connect_options();
    options->path = path;
    net_socket_connect_options(skt, options, connectListener);
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
    if(cb) skt->on_event_end = cb;
}

void net_socket_pause(net_socket_t* skt) {

}

void net_socket_resume(net_socket_t* skt) {

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
    if(list_empty(skt->write_list)) {

    } else {

    }
    return true;
}

/**
 * ------------------------------------------------------------------------------------------------
 */

net_socket_t* net_connect_options(http_t* h, net_socket_options_t *conf, net_socket_connect_options_t *options, on_socket_event cb /*= NULL*/) {
    net_socket_t* ret = net_create_socket(h, conf);

    net_socket_connect_options(ret, options, cb);
    return ret;
}

net_socket_t* net_connect_path(http_t* h, char *path, on_socket_event cb /*= NULL*/) {
    net_socket_t* ret = net_create_socket(h, NULL);

    net_socket_connect_path(ret, path, cb);
    return ret;
}

net_socket_t* net_connect_port(http_t* h, int port, char *host, on_socket_event cb /*= NULL*/) {
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