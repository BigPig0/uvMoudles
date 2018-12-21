#include "public_def.h"
#include "private_def.h"
#include "net.h"
#include "cstl_easy.h"

/**
 * Class: net.Server
 * This class is used to create a TCP or IPC server.
 */
typedef struct _net_server_ {
    bool            allowHalfOpen;    //Indicates whether half-opened TCP connections are allowed. Default: false.
    bool            pauseOnConnect;   //Indicates whether the socket should be paused on incoming connections. Default: false.
    bool            listening;        //<boolean> Indicates whether or not the server is listening for connections.
    int             maxConnections;   //Set this property to reject connections when the server's connection count gets high.
    //private
    http_t*         handle;
    uv_tcp_t        uv_tcp_h;
    on_server_event_connection    on_connection;
    on_server_event               on_close;
    on_event_error                on_error;
    on_server_event               on_listening;
    on_server_get_connections     on_get_connections;
}net_server_t;

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
    int namelen;
    ret = uv_tcp_getsockname(&svr->uv_tcp_h, &name, &namelen);
    if(ret < 0) {
        printf("uv_tcp_getsockname error: %s\n", uv_strerror(ret));
        return 0;
    }
    if(name.sa_family == AF_INET) {

    } else if(name.sa_family == AF_INET6) {

    }
    uv_ip4_name((struct sockaddr_in*)&name, addr, 16);
    
    return ret;
}

void net_server_close(net_server_t* svr, on_server_event cb /*= NULL*/) {
    if(cb) svr->on_close = cb;
}

void net_server_get_connections(net_server_t* svr, on_server_get_connections cb /*= NULL*/) {
    if(cb) svr->on_get_connections = cb;
}

void net_server_listen_handle(net_server_t* svr, void* handle, int backlog /*= 0*/, on_server_event_connection callback /*= NULL*/) {

}

void net_server_listen_options(net_server_t* svr, net_server_listen_options_t* options, on_server_event_connection callback /*= NULL*/) {

}

void net_server_listen_path(net_server_t* svr, char* path, int backlog /*= 0*/, on_server_event_connection callback /*= NULL*/) {

}

void net_server_listen_port(net_server_t* svr, int port, char* host, int backlog /*= 0*/, on_server_event_connection callback /*= NULL*/) {

}

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
    bool        connecting;   //If true, socket.connect(options[, connectListener]) was called and has not yet finished. Will be set to true before emitting 'connect' event and/or calling socket.connect(options[, connectListener])'s callback.
    bool        destroyed;    //Indicates if the connection is destroyed or not. Once a connection is destroyed no further data can be transferred using it.
    bool        isKeepAlive;  //
    int         initialDelay; //<
    bool        noDelay;
    int         timeout;

    char        *localAddress;//The string representation of the local IP address the remote client is connecting on. For example, in a server listening on '0.0.0.0', if a client connects on '192.168.1.1', the value of socket.localAddress would be '192.168.1.1'.
    int         localPort;    //The numeric representation of the local port. For example, 80 or 21.
    bool        pending;      //This is true if the socket is not connected yet, either because .connect() has not yet been called or because it is still in the process of connecting (see socket.connecting).
    char        *remoteAddress;//The string representation of the remote IP address. For example, '74.125.127.100' or '2001:4860:a005::68'. Value may be undefined if the socket is destroyed (for example, if the client disconnected).
    int         remoteFamily; //The string representation of the remote IP family. 'IPv4' or 'IPv6'.
    int         remotePort;   //The numeric representation of the remote port. For example, 80 or 21.

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


net_socket_options_t* net_socket_create_options() {
    SAFE_MALLOC(net_socket_options_t, ret);
    return ret;
}

net_socket_connect_options_t* net_socket_create_connect_options() {
    SAFE_MALLOC(net_socket_connect_options_t, ret);
    ret->host = "localhost";
    ret->family = 4;
    return ret;
}

net_socket_t* net_create_socket(net_socket_options_t *option /*= NULL*/) {
    SAFE_MALLOC(net_socket_t, ret);
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
    net_address_t ret;
    return ret;
}

void net_socket_connect_options(net_socket_t* skt, net_socket_connect_options_t *options, on_socket_event connectListener /*= NULL*/) {

}

void net_socket_connect_path(net_socket_t* skt, char* path, on_socket_event connectListener /*= NULL*/) {

}

void net_socket_connect_port(net_socket_t* skt, int port, char *host /*= NULL*/, on_socket_event connectListener /*= NULL*/) {

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

}

void net_socket_set_no_delay(net_socket_t* skt, bool noDelay /*= true*/) {

}

void net_socket_set_timeout(net_socket_t* skt, int timeout, on_socket_event cb /*= NULL*/) {

}

bool net_socket_write(net_socket_t* skt, char *data, int len, on_socket_event cb /*= NULL*/) {

}

/**
 * ------------------------------------------------------------------------------------------------
 */

net_connect_options_t* net_create_connect_options() {
    SAFE_MALLOC(net_connect_options_t, ret);
    ret->socket_options = net_socket_create_options();
    ret->connect_options = net_socket_create_connect_options();
    return ret;
}

net_socket_t* net_connect_options(net_connect_options_t *options, on_socket_event cb /*= NULL*/) {
    net_socket_t* ret = net_create_socket(options->socket_options);

    net_socket_connect_options(ret, options->connect_options, cb);
    return ret;
}

net_socket_t* net_connect_path(char *path, on_socket_event cb /*= NULL*/) {
    net_socket_t* ret = net_create_socket();

    net_socket_connect_path(ret, path, cb);
    return ret;
}

net_socket_t* net_connect_port(int port, char *host, on_socket_event cb /*= NULL*/) {
    net_socket_t* ret = net_create_socket();

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

}

bool net_is_ipv6(char* input) {

}