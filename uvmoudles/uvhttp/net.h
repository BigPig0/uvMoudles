/*!
 * \file net.h
 * \date 2018/12/19 14:17
 *
 * \author wlla
 * Contact: user@company.com
 *
 * \brief 
 *
 * Node.js v11.5.0 Documentation
 *
 Table of Contents
 Net
    IPC Support
        Identifying paths for IPC connections
    Class: net.Server
         new net.Server([options][, connectionListener])
         Event: 'close'
         Event: 'connection'
         Event: 'error'
         Event: 'listening'
         server.address()
         server.close([callback])
         server.connections
         server.getConnections(callback)
         server.listen()
             server.listen(handle[, backlog][, callback])
             server.listen(options[, callback])
             server.listen(path[, backlog][, callback])
             server.listen([port[, host[, backlog]]][, callback])
             server.listening
         server.maxConnections
         server.ref()
         server.unref()
    Class: net.Socket
         new net.Socket([options])
         Event: 'close'
         Event: 'connect'
         Event: 'data'
         Event: 'drain'
         Event: 'end'
         Event: 'error'
         Event: 'lookup'
         Event: 'ready'
         Event: 'timeout'
         socket.address()
         socket.bufferSize
         socket.bytesRead
         socket.bytesWritten
         socket.connect()
            socket.connect(options[, connectListener])
            socket.connect(path[, connectListener])
            socket.connect(port[, host][, connectListener])
         socket.connecting
         socket.destroy([exception])
         socket.destroyed
         socket.end([data][, encoding][, callback])
         socket.localAddress
         socket.localPort
         socket.pause()
         socket.pending
         socket.ref()
         socket.remoteAddress
         socket.remoteFamily
         socket.remotePort
         socket.resume()
         socket.setEncoding([encoding])
         socket.setKeepAlive([enable][, initialDelay])
         socket.setNoDelay([noDelay])
         socket.setTimeout(timeout[, callback])
         socket.unref()
         socket.write(data[, encoding][, callback])
    net.connect()
         net.connect(options[, connectListener])
         net.connect(path[, connectListener])
         net.connect(port[, host][, connectListener])
    net.createConnection()
        net.createConnection(options[, connectListener])
        net.createConnection(path[, connectListener])
        net.createConnection(port[, host][, connectListener])
    net.createServer([options][, connectionListener])
    net.isIP(input)
    net.isIPv4(input)
    net.isIPv6(input)
*/

#ifndef _UV_NET_
#define _UV_NET_ 

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _net_address_ {
    char      address[20];
    int       family;
    int       port;
}net_address_t;

/**
 * Class: net.Server
 * This class is used to create a TCP or IPC server.
 */

typedef struct _net_server_options_ {
    bool    allowHalfOpen;  //Indicates whether half-opened TCP connections are allowed. Default: false.
    bool    pauseOnConnect; //Indicates whether the socket should be paused on incoming connections. Default: false.
}net_server_options_t;

typedef struct _net_server_listen_options_ {
    int       port;
    char*     host;
    char*     path;        //Will be ignored if port is specified. See Identifying paths for IPC connections.
    int       backlog;     //Common parameter of server.listen() functions.
    bool      exclusive;   //Default: false
    bool      readableAll; //For IPC servers makes the pipe readable for all users. Default: false
    bool      writableAll; //For IPC servers makes the pipe writable for all users. Default: false
    bool      ipv6Only;    //For TCP servers, setting ipv6Only to true will disable dual-stack support, i.e., binding to host :: won't make 0.0.0.0 be bound. Default: false.
}net_server_listen_options_t;
extern net_server_listen_options_t* net_server_create_listen_options();

typedef struct _net_server_  net_server_t;

typedef void (*on_server_event_connection)(net_server_t* svr, net_socket_t* client);
typedef void (*on_server_event)(net_server_t* svr);

/**
 * net.createServer([options][, connectionListener])
 * options <Object>
 *   allowHalfOpen <boolean> Indicates whether half-opened TCP connections are allowed. Default: false.
 *   pauseOnConnect <boolean> Indicates whether the socket should be paused on incoming connections. Default: false.
 * connectionListener <Function> Automatically set as a listener for the 'connection' event.
 * Returns: <net.Server>
 * Creates a new TCP or IPC server.
 *
 * If allowHalfOpen is set to true, when the other end of the socket sends a FIN packet, the server will only send a FIN packet back when socket.end() is explicitly called, until then the connection is half-closed (non-readable but still writable). See 'end' event and RFC 1122 (section 4.2.2.13) for more information.
 * If pauseOnConnect is set to true, then the socket associated with each incoming connection will be paused, and no data will be read from its handle. This allows connections to be passed between processes without any data being read by the original process. To begin reading data from a paused socket, call socket.resume().
 * The server can be a TCP server or an IPC server, depending on what it listen() to.
 */
extern net_server_t* net_create_server(http_t* h, net_server_options_t* options = NULL, on_server_event_connection connectionListener = NULL);

/**
 * Event: 'close'
 * Emitted when the server closes. Note that if connections exist, this event is not emitted until all connections are ended.
 */
extern void net_server_on_close(net_server_t* svr, on_server_event cb);

/**
 * Event: 'connection'
 * <net.Socket> The connection object
 * Emitted when a new connection is made. socket is an instance of net.Socket.
 */
extern void net_server_on_connection(net_server_t* svr, on_server_event_connection cb);

/**
 * Event: 'error'
 * <Error>
 * Emitted when an error occurs. Unlike net.Socket, the 'close' event will not be emitted directly following this event unless server.close() is manually called. See the example in discussion of
 */
typedef void (*on_event_error)(net_server_t* svr, int error);
extern void net_server_on_error(net_server_t* svr, on_event_error cb);

/**
 * Event: 'listening'
 * Emitted when the server has been bound after calling server.listen().
 */
extern void net_server_on_listening(net_server_t* svr, on_server_event cb);

/**
 * server.address()
 * Returns: <Object> | <string>
 * Returns the bound address, the address family name, and port of the server as reported by the operating system if listening on an IP socket (useful to find which port was assigned when getting an OS-assigned address): { port: 12346, family: 'IPv4', address: '127.0.0.1' }.
 */
extern int net_server_address(net_server_t* svr, net_address_t* address);

/**
 * server.close([callback])
 * callback <Function> Called when the server is closed
 * Returns: <net.Server>
 * Stops the server from accepting new connections and keeps existing connections. This function is asynchronous, the server is finally closed when all connections are ended and the server emits a 'close' event. The optional callback will be called once the 'close' event occurs. Unlike that event, it will be called with an Error as its only argument if the server was not open when it was closed.
 */
extern void net_server_close(net_server_t* svr, on_server_event cb = NULL);

/**
 * server.getConnections(callback)
 * callback <Function>
 * Returns: <net.Server>
 * Asynchronously get the number of concurrent connections on the server. Works when sockets were sent to forks.
 * Callback should take two arguments err and count.
 */
typedef void (*on_server_get_connections)(net_server_t* svr, int error, int num);
extern void net_server_get_connections(net_server_t* svr, on_server_get_connections cb = NULL);

/**
 * server.listen()
 * Start a server listening for connections. A net.Server can be a TCP or an IPC server depending on what it listens to.
 * Possible signatures:
 *   server.listen(handle[, backlog][, callback])
 *   server.listen(options[, callback])
 *   server.listen(path[, backlog][, callback]) for IPC servers
 *   server.listen([port[, host[, backlog]]][, callback]) for TCP servers
 * This function is asynchronous. When the server starts listening, the 'listening' event will be emitted. The last parameter callback will be added as a listener for the 'listening' event.
 * All listen() methods can take a backlog parameter to specify the maximum length of the queue of pending connections. The actual length will be determined by the OS through sysctl settings such as tcp_max_syn_backlog and somaxconn on Linux. The default value of this parameter is 511 (not 512).
 * All net.Socket are set to SO_REUSEADDR (see socket(7) for details).
 * The server.listen() method can be called again if and only if there was an error during the first server.listen() call or server.close() has been called. Otherwise, an ERR_SERVER_ALREADY_LISTEN error will be thrown.
 * One of the most common errors raised when listening is EADDRINUSE. This happens when another server is already listening on the requested port/path/handle. One way to handle this would be to retry after a certain amount of time:
 */
/**
 * server.listen(handle[, backlog][, callback])
 * handle <Object>
 * backlog <number> Common parameter of server.listen() functions
 * callback <Function> Common parameter of server.listen() functions
 * Returns: <net.Server>
 * Start a server listening for connections on a given handle that has already been bound to a port, a UNIX domain socket, or a Windows named pipe.
 * The handle object can be either a server, a socket (anything with an underlying _handle member), or an object with an fd member that is a valid file descriptor.
 * Listening on a file descriptor is not supported on Windows.
 */
extern void net_server_listen_handle(net_server_t* svr, void* handle, int backlog = 0, on_server_event_connection callback = NULL);

/**
 * server.listen(options[, callback])
 * History
 * options <Object> Required. Supports the following properties:
 *   port <number>
 *   host <string>
 *   path <string> Will be ignored if port is specified. See Identifying paths for IPC connections.
 *   backlog <number> Common parameter of server.listen() functions.
 *   exclusive <boolean> Default: false
 *   readableAll <boolean> For IPC servers makes the pipe readable for all users. Default: false
 *   writableAll <boolean> For IPC servers makes the pipe writable for all users. Default: false
 *   ipv6Only <boolean> For TCP servers, setting ipv6Only to true will disable dual-stack support, i.e., binding to host :: won't make 0.0.0.0 be bound. Default: false.
 * callback <Function> Common parameter of server.listen() functions.
 * Returns: <net.Server>
 * If port is specified, it behaves the same as server.listen([port[, host[, backlog]]][, callback]). Otherwise, if path is specified, it behaves the same as server.listen(path[, backlog][, callback]). If none of them is specified, an error will be thrown.
 * If exclusive is false (default), then cluster workers will use the same underlying handle, allowing connection handling duties to be shared. When exclusive is true, the handle is not shared, and attempted port sharing results in an error. An example which listens on an exclusive port is shown below.
 * Starting an IPC server as root may cause the server path to be inaccessible for unprivileged users. Using readableAll and writableAll will make the server accessible for all users.
 */
extern void net_server_listen_options(net_server_t* svr, net_server_listen_options_t* options, on_server_event_connection callback = NULL);

/**
 * server.listen(path[, backlog][, callback])
 * path <string> Path the server should listen to. See Identifying paths for IPC connections.
 * backlog <number> Common parameter of server.listen() functions.
 * callback <Function> Common parameter of server.listen() functions.
 * Returns: <net.Server>
 * Start an IPC server listening for connections on the given path.
 */
extern void net_server_listen_path(net_server_t* svr, char* path, int backlog = 0, on_server_event_connection callback = NULL);

/**
 * server.listen([port[, host[, backlog]]][, callback])
 * port <number>
 * host <string>
 * backlog <number> Common parameter of server.listen() functions.
 * callback <Function> Common parameter of server.listen() functions.
 * Returns: <net.Server>
 * Start a TCP server listening for connections on the given port and host.
 * If port is omitted or is 0, the operating system will assign an arbitrary unused port, which can be retrieved by using server.address().port after the 'listening' event has been emitted.
 * If host is omitted, the server will accept connections on the unspecified IPv6 address (::) when IPv6 is available, or the unspecified IPv4 address (0.0.0.0) otherwise.
 * In most operating systems, listening to the unspecified IPv6 address (::) may cause the net.Server to also listen on the unspecified IPv4 address (0.0.0.0).
 */
extern void net_server_listen_port(net_server_t* svr, int port, char* host, int backlog = 0, on_server_event_connection callback = NULL);

/**
 * -------------------------------------------------------------------------------------------------------------------------------------------
 */

/**
 * Class: net.Socket
 * This class is an abstraction of a TCP socket or a streaming IPC endpoint (uses named pipes on Windows, and UNIX domain sockets otherwise). A net.Socket is also a duplex stream, so it can be both readable and writable, and it is also an EventEmitter.

 * A net.Socket can be created by the user and used directly to interact with a server. For example, it is returned by net.createConnection(), so the user can use it to talk to the server.

 * It can also be created by Node.js and passed to the user when a connection is received. For example, it is passed to the listeners of a 'connection' event emitted on a net.Server, so the user can use it to interact with the client.
 */

typedef struct _net_socket_options_{
    int    fd;            //If specified, wrap around an existing socket with the given file descriptor, otherwise a new socket will be created.
    bool   allowHalfOpen; //Indicates whether half-opened TCP connections are allowed. See net.createServer() and the 'end' event for details. Default: false.
    bool   readable;      //Allow reads on the socket when an fd is passed, otherwise ignored. Default: false.
    bool   writable;      //Allow writes on the socket when an fd is passed, otherwise ignored. Default: false.
}net_socket_options_t;
extern net_socket_options_t* net_socket_create_options();

typedef struct _net_socket_connect_options_ {
    //For TCP connections, available options are:
    int    port;          //Required. Port the socket should connect to.
    char   *host;         //Host the socket should connect to. Default: 'localhost'.
    char   *localAddress; //Local address the socket should connect from.
    int    localPort;     //Local port the socket should connect from.
    int    family;        //Version of IP stack, can be either 4 or 6. Default: 4.
    //int    hints;         //Optional dns.lookup() hints.
    //dns_lookup lookup;    //Custom lookup function. Default: dns.lookup().
    //For IPC connections, available options are:
    char   *path;         //Required. Path the client should connect to. See Identifying paths for IPC connections. If provided, the TCP-specific options above are ignored.
}net_socket_connect_options_t;
extern net_socket_connect_options_t* net_socket_create_connect_options();

typedef struct _net_socket_  net_socket_t;

typedef void (*on_socket_event)(net_socket_t* skt);

/**
 * new net.Socket([options])
 * options <Object> Available options are:
 *    fd <number> If specified, wrap around an existing socket with the given file descriptor, otherwise a new socket will be created.
 *    allowHalfOpen <boolean> Indicates whether half-opened TCP connections are allowed. See net.createServer() and the 'end' event for details. Default: false.
 *    readable <boolean> Allow reads on the socket when an fd is passed, otherwise ignored. Default: false.
 *    writable <boolean> Allow writes on the socket when an fd is passed, otherwise ignored. Default: false.
 * Returns: <net.Socket>
 * Creates a new socket object.
 * The newly created socket can be either a TCP socket or a streaming IPC endpoint, depending on what it connect() to.
 */
extern net_socket_t* net_create_socket(net_socket_options_t *option = NULL);

/**
 * Event: 'close'
 * hadError <boolean> true if the socket had a transmission error.
 * Emitted once the socket is fully closed. The argument hadError is a boolean which says if the socket was closed due to a transmission error.
 */
typedef void (*on_socket_event_close)(net_socket_t* skt, bool had_error);
extern void net_socket_on_close(net_socket_t* skt, on_socket_event_close cb);

/**
 * Event: 'connect'
 * Emitted when a socket connection is successfully established. See net.createConnection().
 */
extern void net_socket_on_connect(net_socket_t* skt, on_socket_event cb);

/**
 * Event: 'data'
 * <Buffer> | <string>
 * Emitted when data is received. The argument data will be a Buffer or String. Encoding of data is set by socket.setEncoding().
 * Note that the data will be lost if there is no listener when a Socket emits a 'data' event.
 */
typedef void (*on_socket_event_data)(net_socket_t* skt, char *data, int len);
extern void net_socket_on_data(net_socket_t* skt, on_socket_event_data cb);

/**
 * Event: 'drain'
 * Emitted when the write buffer becomes empty. Can be used to throttle uploads.
 * See also: the return values of socket.write().
 */
extern void net_socket_on_drain(net_socket_t* skt, on_socket_event cb);

/**
 * Event: 'end'
 * Emitted when the other end of the socket sends a FIN packet, thus ending the readable side of the socket.
 * By default (allowHalfOpen is false) the socket will send a FIN packet back and destroy its file descriptor once it has written out its pending write queue. 
 * However, if allowHalfOpen is set to true, the socket will not automatically end() its writable side, allowing the user to write arbitrary amounts of data. The user must call end() explicitly to close the connection (i.e. sending a FIN packet back).
 */
extern void net_socket_on_end(net_socket_t* skt, on_socket_event cb);

/**
 * Event: 'error'
 * <Error>
 * Emitted when an error occurs. The 'close' event will be called directly following this event.
 */
typedef void (*on_socket_event_error)(net_socket_t* skt, int error);
extern void net_socket_on_error(net_socket_t* skt, on_socket_event_error cb);

/**
 * Event: 'lookup'
 * Emitted after resolving the hostname but before connecting. Not applicable to UNIX sockets.

 * err <Error> | <null> The error object. See dns.lookup().
 * address <string> The IP address.
 * family <string> | <null> The address type. See dns.lookup().
 * host <string> The hostname.
 */
typedef void (*on_socket_event_lookup)(net_socket_t* skt, int error, char *address,char *family, char* host);
extern void net_socket_on_lookup(net_socket_t* skt, on_socket_event_lookup cb);

/**
 * Event: 'ready'
 * Emitted when a socket is ready to be used.
 * Triggered immediately after 'connect'.
 */
extern void net_socket_on_ready(net_socket_t* skt, on_socket_event cb);

/**
 * Event: 'timeout'
 * Emitted if the socket times out from inactivity. This is only to notify that the socket has been idle. The user must manually close the connection.
 * See also: socket.setTimeout().
 */
extern void net_socket_on_timeout(net_socket_t* skt, on_socket_event cb);

/**
 * socket.address()
 * Returns: <Object>
 * Returns the bound address, the address family name and port of the socket as reported by the operating system: { port: 12346, family: 'IPv4', address: '127.0.0.1' }
 */
extern net_address_t net_socket_address(net_socket_t* skt);

/**
 * socket.connect()
 * Initiate a connection on a given socket.
 * Possible signatures:
 *     socket.connect(options[, connectListener])
 *     socket.connect(path[, connectListener]) for IPC connections.
 *     socket.connect(port[, host][, connectListener]) for TCP connections.
 * Returns: <net.Socket> The socket itself.
 * This function is asynchronous. When the connection is established, the 'connect' event will be emitted. If there is a problem connecting, instead of a 'connect' event, an 'error' event will be emitted with the error passed to the 'error' listener. The last parameter connectListener, if supplied, will be added as a listener for the 'connect' event once.
 */
/**
 * socket.connect(options[, connectListener])
 * options <Object>
 * connectListener <Function> Common parameter of socket.connect() methods. Will be added as a listener for the 'connect' event once.
 * Returns: <net.Socket> The socket itself.
 * Initiate a connection on a given socket. Normally this method is not needed, the socket should be created and opened with net.createConnection(). Use this only when implementing a custom Socket.
 */
extern void net_socket_connect_options(net_socket_t* skt, net_socket_connect_options_t *options, on_socket_event connectListener = NULL);

/**
 * socket.connect(path[, connectListener])
 * path <string> Path the client should connect to. See Identifying paths for IPC connections.
 * connectListener <Function> Common parameter of socket.connect() methods. Will be added as a listener for the 'connect' event once.
 * Returns: <net.Socket> The socket itself.
 * Initiate an IPC connection on the given socket.
 *
 * Alias to socket.connect(options[, connectListener]) called with { path: path } as options.
 */
extern void net_socket_connect_path(net_socket_t* skt, char* path, on_socket_event connectListener = NULL);

/**
 * socket.connect(port[, host][, connectListener])
 * port <number> Port the client should connect to.
 * host <string> Host the client should connect to.
 * connectListener <Function> Common parameter of socket.connect() methods. Will be added as a listener for the 'connect' event once.
 * Returns: <net.Socket> The socket itself.
 * Initiate a TCP connection on the given socket.
 *
 * Alias to socket.connect(options[, connectListener]) called with {port: port, host: host} as options.
 */
extern void net_socket_connect_port(net_socket_t* skt, int port, char *host = NULL, on_socket_event connectListener = NULL);

/**
 * socket.destroy([exception])
 * exception <Object>
 * Returns: <net.Socket>
 * Ensures that no more I/O activity happens on this socket. Only necessary in case of errors (parse error or so).
 *
 * If exception is specified, an 'error' event will be emitted and any listeners for that event will receive exception as an argument.
 */
extern void net_socket_destory(net_socket_t* skt, int err = 0);

/**
 * socket.end([data][, encoding][, callback])
 * data <string> | <Buffer> | <Uint8Array>
 * encoding <string> Only used when data is string. Default: 'utf8'.
 * callback <Function> Optional callback for when the socket is finished.
 * Returns: <net.Socket> The socket itself.
 * Half-closes the socket. i.e., it sends a FIN packet. It is possible the server will still send some data.
 *
 * If data is specified, it is equivalent to calling socket.write(data, encoding) followed by socket.end().
 */
extern void net_socket_end(net_socket_t* skt, char* data = NULL, int len = 0, on_socket_event cb = NULL);

/**
 * socket.pause()
 * Returns: <net.Socket> The socket itself.
 * Pauses the reading of data. That is, 'data' events will not be emitted. Useful to throttle back an upload.
 */
extern void net_socket_pause(net_socket_t* skt);

/**
 * socket.resume()
 * Returns: <net.Socket> The socket itself.
 * Resumes reading after a call to socket.pause().
 */
extern void net_socket_resume(net_socket_t* skt);

/**
 * socket.setKeepAlive([enable][, initialDelay])
 * enable <boolean> Default: false
 * initialDelay <number> Default: 0
 * Returns: <net.Socket> The socket itself.
 * Enable/disable keep-alive functionality, and optionally set the initial delay before the first keepalive probe is sent on an idle socket.
 *
 * Set initialDelay (in milliseconds) to set the delay between the last data packet received and the first keepalive probe. Setting 0 for initialDelay will leave the value unchanged from the default (or previous) setting.
 */
extern void net_socket_set_keep_alive(net_socket_t* skt, bool enable = false, int initialDelay = 0);

/**
 * socket.setNoDelay([noDelay])
 * noDelay <boolean> Default: true
 * Returns: <net.Socket> The socket itself.
 * Disables the Nagle algorithm. By default TCP connections use the Nagle algorithm, they buffer data before sending it off. Setting true for noDelay will immediately fire off data each time socket.write() is called.
 */
extern void net_socket_set_no_delay(net_socket_t* skt, bool noDelay = true);

/**
 * socket.setTimeout(timeout[, callback])
 * timeout <number>
 * callback <Function>
 * Returns: <net.Socket> The socket itself.
 * Sets the socket to timeout after timeout milliseconds of inactivity on the socket. By default net.Socket do not have a timeout.
 */
extern void net_socket_set_timeout(net_socket_t* skt, int timeout, on_socket_event cb = NULL);

/**
 * socket.write(data[, encoding][, callback])
 * data <string> | <Buffer> | <Uint8Array>
 * encoding <string> Only used when data is string. Default: utf8.
 * callback <Function>
 * Returns: <boolean>
 * Sends data on the socket. The second parameter specifies the encoding in the case of a string ¡ª it defaults to UTF8 encoding.
 *
 * Returns true if the entire data was flushed successfully to the kernel buffer. Returns false if all or part of the data was queued in user memory. 'drain' will be emitted when the buffer is again free.
 * The optional callback parameter will be executed when the data is finally written out - this may not be immediately.
 *
 * See Writable stream write() method for more information.
 */
extern bool net_socket_write(net_socket_t* skt, char *data, int len, on_socket_event cb = NULL);


/**
 * ----------------------------------------------------------------------------------------------------
 */


typedef struct _net_connect_options_ {
    net_socket_options_t*            socket_options;
    net_socket_connect_options_t*    connect_options;
}net_connect_options_t;
extern net_connect_options_t* net_create_connect_options();

/**
 * net.connect()
 * Aliases to net_create_socket.
 *
 * Possible signatures:
 * net.connect(options[, connectListener])
 * net.connect(path[, connectListener]) for IPC connections.
 * net.connect(port[, host][, connectListener]) for TCP connections.
 */
/**
 * net.connect(options[, connectListener])
 * options <Object>
 * connectListener <Function>
 * Alias to net.createConnection(options[, connectListener]).
 */
extern net_socket_t* net_connect_options(net_connect_options_t *options, on_socket_event cb = NULL);

/**
 * net.connect(path[, connectListener])
 * Added in: v0.1.90
 * path <string>
 * connectListener <Function>
 * Alias to net.createConnection(path[, connectListener]).
 */
extern net_socket_t* net_connect_path(char *path, on_socket_event cb = NULL);

/**
 * net.connect(port[, host][, connectListener])
 * Added in: v0.1.90
 * port <number>
 * host <string>
 * connectListener <Function>
 */
extern net_socket_t* net_connect_port(int port, char *host, on_socket_event cb = NULL);

/**
 * net.isIP(input)
 * input <string>
 * Returns: <integer>
 * Tests if input is an IP address. Returns 0 for invalid strings, returns 4 for IP version 4 addresses, and returns 6 for IP version 6 addresses.
 */
extern int net_is_ip(char* input);

/**
 * net.isIPv4(input)
 * input <string>
 * Returns: <boolean>
 * Returns true if input is a version 4 IP address, otherwise returns false.
 */
extern bool net_is_ipv4(char* input);

/**
 * net.isIPv6(input)
 * input <string>
 * Returns: <boolean>
 * Returns true if input is a version 6 IP address, otherwise returns false.
 */
extern bool net_is_ipv6(char* input);

#ifdef __cplusplus
}
#endif

#endif