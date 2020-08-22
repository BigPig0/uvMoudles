/*!
 * \file http.h
 * \date 2019/01/05 13:01
 *
 * \author wlla
 * Contact: user@company.com
 *
 * \brief 
 *
 * Node.js v11.6.0 Documentation
 *
 Table of Contents
 HTTP
    Class: http.Agent
       new Agent([options])
       agent.createConnection(options[, callback])
       agent.keepSocketAlive(socket)
       agent.reuseSocket(socket, request)
       agent.destroy()
       agent.freeSockets
       agent.getName(options)
       agent.maxFreeSockets
       agent.maxSockets
       agent.requests
       agent.sockets
   Class: http.ClientRequest
       Event: 'abort'
       Event: 'connect'
       Event: 'continue'
       Event: 'information'
       Event: 'response'
       Event: 'socket'
       Event: 'timeout'
       Event: 'upgrade'
       request.abort()
       request.aborted
       request.connection
       request.end([data[, encoding]][, callback])
       request.finished
       request.flushHeaders()
       request.getHeader(name)
       request.maxHeadersCount
       request.removeHeader(name)
       request.setHeader(name, value)
       request.setNoDelay([noDelay])
       request.setSocketKeepAlive([enable][, initialDelay])
       request.setTimeout(timeout[, callback])
       request.socket
       request.write(chunk[, encoding][, callback])
    Class: http.Server
       Event: 'checkContinue'
       Event: 'checkExpectation'
       Event: 'clientError'
       Event: 'close'
       Event: 'connect'
       Event: 'connection'
       Event: 'request'
       Event: 'upgrade'
       server.close([callback])
       server.headersTimeout
       server.listen()
       server.listening
       server.maxHeadersCount
       server.setTimeout([msecs][, callback])
       server.timeout
       server.keepAliveTimeout
    Class: http.ServerResponse
       Event: 'close'
       Event: 'finish'
       response.addTrailers(headers)
       response.connection
       response.end([data][, encoding][, callback])
       response.finished
       response.getHeader(name)
       response.getHeaderNames()
       response.getHeaders()
       response.hasHeader(name)
       response.headersSent
       response.removeHeader(name)
       response.sendDate
       response.setHeader(name, value)
       response.setTimeout(msecs[, callback])
       response.socket
       response.statusCode
       response.statusMessage
       response.write(chunk[, encoding][, callback])
       response.writeContinue()
       response.writeHead(statusCode[, statusMessage][, headers])
       response.writeProcessing()
    Class: http.IncomingMessage
       Event: 'aborted'
       Event: 'close'
       message.aborted
       message.complete
       message.destroy([error])
       message.headers
       message.httpVersion
       message.method
       message.rawHeaders
       message.rawTrailers
       message.setTimeout(msecs, callback)
       message.socket
       message.statusCode
       message.statusMessage
       message.trailers
       message.url
       http.METHODS
       http.STATUS_CODES
       http.createServer([options][, requestListener])
       http.get(options[, callback])
       http.get(url[, options][, callback])
       http.globalAgent
       http.maxHeaderSize
       http.request(options[, callback])
       http.request(url[, options][, callback])
*/

#ifndef _UV_HTTP_
#define _UV_HTTP_

#ifdef __cplusplus
extern "C" {
#endif

#include "net.h"

typedef struct _http_agent_             http_agent_t;
typedef struct _http_client_request_    http_client_request_t;
typedef struct _http_server_            http_server_t;
typedef struct _http_server_response_   http_server_response_t;
typedef struct _http_incoming_message_  http_incoming_message_t;

/**
 * Class: http.Agent
 * An Agent is responsible for managing connection persistence and reuse for HTTP clients.
 */

/**
 * new Agent([options])
 * options <字符串数组 {"key","value","key","value",NULL}> Set of configurable options to set on the agent. Can have the following fields:
 * * keepAlive <boolean> Keep sockets around even when there are no outstanding requests, so they can be used for future requests without having to reestablish a TCP connection. Default: false.
 * * keepAliveMsecs <number> When using the keepAlive option, specifies the initial delay for TCP Keep-Alive packets. Ignored when the keepAlive option is false or undefined. Default: 1000.
 * * maxSockets <number> Maximum number of sockets to allow per host. Default: Infinity.
 * * maxFreeSockets <number> Maximum number of sockets to leave open in a free state. Only relevant if keepAlive is set to true. Default: 256.
 * * timeout <number> Socket timeout in milliseconds. This will set the timeout when the socket is created.
 * * options in socket.connect() are also supported.
 * The default http.globalAgent that is used by http.request() has all of these values set to their respective defaults.
 * To configure any of them, a custom http.Agent instance must be created.
 */
http_agent_t* http_create_agent(uv_node_t *h, char **options);

/**
 * agent.createConnection(options[, callback])
 * options <Object> Options containing connection details. Check net.createConnection() for the format of the options
 * callback <Function> Callback function that receives the created socket
 * Returns: <net.Socket>
 * Produces a socket/stream to be used for HTTP requests.
 * By default, this function is the same as net.createConnection(). However, custom agents may override this method in case greater flexibility is desired.
 * A socket/stream can be supplied in one of two ways: by returning the socket/stream from this function, or by passing the socket/stream to callback.
 * callback has a signature of (err, stream).
 */
net_socket_t* http_agent_create_connection(http_agent_t* agent, char **options, on_socket_event cb /*= NULL*/);

/**
 * agent.keepSocketAlive(socket)
 * socket <net.Socket>
 * Called when socket is detached from a request and could be persisted by the Agent. Default behavior is to:
 * socket.setKeepAlive(true, this.keepAliveMsecs);
 * socket.unref();
 * return true;
 * This method can be overridden by a particular Agent subclass. If this method returns a falsy value, the socket will be destroyed instead of persisting it for use with the next request.
 */
bool http_agent_keep_socket_alive(http_agent_t* agent, net_socket_t* socket);

/**
 * agent.reuseSocket(socket, request)
 * socket <net.Socket>
 * request <http.ClientRequest>
 * Called when socket is attached to request after being persisted because of the keep-alive options. Default behavior is to:
 * socket.ref();
 * This method can be overridden by a particular Agent subclass.
 */
void http_agent_reuse_socket(http_agent_t* agent, net_socket_t *socket, http_client_request_t *request);

/**
 * agent.destroy()
 * Destroy any sockets that are currently in use by the agent.
 * It is usually not necessary to do this. However, if using an agent with keepAlive enabled, then it is best to explicitly shut down the agent when it will no longer be used. Otherwise, sockets may hang open for quite a long time before the server terminates them.
 */
void http_agent_destory(http_agent_t* agent);

/**
 * agent.getName(options)
 * options <Object> A set of options providing information for name generation
 * * host <string> A domain name or IP address of the server to issue the request to
 * * port <number> Port of remote server
 * * localAddress <string> Local interface to bind for network connections when issuing the request
 * * family <integer> Must be 4 or 6 if this doesn't equal undefined.
 * Returns: <string>
 * Get a unique name for a set of request options, to determine whether a connection can be reused. For an HTTP agent, this returns host:port:localAddress or host:port:localAddress:family. For an HTTPS agent, the name includes the CA, cert, ciphers, and other HTTPS/TLS-specific options that determine socket reusability.
 */
char* http_agent_get_name(char* buff, char* host, int port, char* local_address, int family);

//////////////////////////////////////////////////////////////////////////

/**
 * Class: http.ClientRequest
 * This object is created internally and returned from http.request(). It represents an in-progress request whose header has already been queued. The header is still mutable using the setHeader(name, value), getHeader(name), removeHeader(name) API. The actual header will be sent along with the first data chunk or when calling request.end().
 * To get the response, add a listener for 'response' to the request object. 'response' will be emitted from the request object when the response headers have been received. The 'response' event is executed with one argument which is an instance of http.IncomingMessage.
 * During the 'response' event, one can add listeners to the response object; particularly to listen for the 'data' event.
 * If no 'response' handler is added, then the response will be entirely discarded. However, if a 'response' event handler is added, then the data from the response object must be consumed, either by calling response.read() whenever there is a 'readable' event, or by adding a 'data' handler, or by calling the .resume() method. Until the data is consumed, the 'end' event will not fire. Also, until the data is read it will consume memory that can eventually lead to a 'process out of memory' error.
 * Node.js does not check whether Content-Length and the length of the body which has been transmitted are equal or not.
 */

/**
 * Event: 'abort'
 * Emitted when the request has been aborted by the client. This event is only emitted on the first call to abort().
 */
typedef void (*http_client_request_abort_cb)(http_client_request_t *request, int error);
void http_client_request_on_abort(http_client_request_t *request, http_client_request_abort_cb cb);

/**
 * Event: 'connect'
 * response <http.IncomingMessage>
 * socket <net.Socket>
 * head <Buffer>
 * Emitted each time a server responds to a request with a CONNECT method. If this event is not being listened for, clients receiving a CONNECT method will have their connections closed.
 */
typedef void(*http_client_request_connect_cb)(http_client_request_t *request, 
                                              http_incoming_message_t* response,
                                              net_socket_t* socket,
                                              char* head);
void http_client_request_on_connect(http_client_request_t *request, http_client_request_connect_cb cb);

/**
 * Event: 'continue'#
 * Emitted when the server sends a '100 Continue' HTTP response, usually because the request contained 'Expect: 100-continue'. This is an instruction that the client should send the request body.
 */
typedef void(*http_client_request_continue_cb)(http_client_request_t *request);
void http_client_request_on_continue(http_client_request_t *request, http_client_request_continue_cb cb);


/**
 * Event: 'information'#
 * Emitted when the server sends a 1xx response (excluding 101 Upgrade). This event is emitted with a callback containing an object with a status code.
 * 101 Upgrade statuses do not fire this event due to their break from the traditional HTTP request/response chain, such as web sockets, in-place TLS upgrades, or HTTP 2.0. To be notified of 101 Upgrade notices, listen for the 'upgrade' event instead.
 */
typedef void(*http_client_request_information_cb)(http_client_request_t *request, http_incoming_message_t* response);
void http_client_request_on_information(http_client_request_t *request, http_client_request_information_cb cb);

/**
 * Event: 'response'#
 * response <http.IncomingMessage>
 * Emitted when a response is received to this request. This event is emitted only once.
 */
typedef void(*http_client_request_response_cb)(http_client_request_t *request, http_incoming_message_t* response);
void http_client_request_on_response(http_client_request_t *request, http_client_request_response_cb cb);

/**
 * Event: 'socket'#
 * socket <net.Socket>
 * Emitted after a socket is assigned to this request.
 */
typedef void(*http_client_request_socket_cb)(http_client_request_t *request);
void http_client_request_on_socket(http_client_request_t *request, http_client_request_socket_cb cb);

/**
 * Event: 'timeout'#
 * Emitted when the underlying socket times out from inactivity. This only notifies that the socket has been idle. The request must be aborted manually.
 * See also: request.setTimeout().
 */
typedef void(*http_client_request_timeout_cb)(http_client_request_t *request);
void http_client_request_on_timeout(http_client_request_t *request, http_client_request_timeout_cb cb);

/**
 * Event: 'upgrade'#
 * response <http.IncomingMessage>
 * socket <net.Socket>
 * head <Buffer>
 * Emitted each time a server responds to a request with an upgrade. If this event is not being listened for and the response status code is 101 Switching Protocols, clients receiving an upgrade header will have their connections closed.
 * A client server pair demonstrating how to listen for the 'upgrade' event.
 */
typedef void(*http_client_request_upgrade_cb)(http_client_request_t *request, 
                                              http_incoming_message_t* response,
                                              net_socket_t* socket,
                                              char* head);
void http_client_request_on_upgrade(http_client_request_t *request, http_client_request_upgrade_cb cb);

/**
 * request.abort()#
 * Marks the request as aborting. Calling this will cause remaining data in the response to be dropped and the socket to be destroyed.
 */
void http_client_request_abort(http_client_request_t *request);

/**
 * request.end([data[, encoding]][, callback])#
 * data <string> | <Buffer>
 * encoding <string>
 * callback <Function>
 * Returns: <this>
 * Finishes sending the request. If any parts of the body are unsent, it will flush them to the stream. If the request is chunked, this will send the terminating '0\r\n\r\n'.
 * If data is specified, it is equivalent to calling request.write(data, encoding) followed by request.end(callback).
 * If callback is specified, it will be called when the request stream is finished.
 */
typedef void(*http_client_request_end_cb)(http_client_request_t *request);
void http_client_request_end(http_client_request_t *request, char* data, int len, http_client_request_end_cb cb);

/**
 * request.flushHeaders()
 * Flush the request headers.
 * For efficiency reasons, Node.js normally buffers the request headers until request.end() is called or the first chunk of request data is written. It then tries to pack the request headers and data into a single TCP packet.
 * That's usually desired (it saves a TCP round-trip), but not when the first data is not sent until possibly much later. request.flushHeaders() bypasses the optimization and kickstarts the request.
 */
void http_client_request_flush_headers(http_client_request_t *request);

/**
 * request.getHeader(name)
 * name <string>
 * Returns: <any>
 * Reads out a header on the request. Note that the name is case insensitive. The type of the return value depends on the arguments provided to request.setHeader().
 */
int http_client_request_get_header(http_client_request_t *request, char *name, char **value);

/**
 * request.removeHeader(name)
 * name <string>
 * Removes a header that's already defined into headers object.
 */
void http_client_request_remove_header(http_client_request_t *request, char *name);

/**
 * request.setHeader(name, value)
 * name <string>
 * value <any>
 * Sets a single header value for headers object. If this header already exists in the to-be-sent headers, its value will be replaced. Use an array of strings here to send multiple headers with the same name. Non-string values will be stored without modification. Therefore, request.getHeader() may return non-string values. However, the non-string values will be converted to strings for network transmission.
 */
void http_client_request_set_header(http_client_request_t *request, char *name, char *value);
/**
 * value字符串数组，以NULL结束，如{"value","value",NULL}
 */
void http_client_request_set_header2(http_client_request_t *request, char *name, char **values);

/**
 * request.setNoDelay([noDelay])
 * noDelay <boolean>
 * Once a socket is assigned to this request and is connected socket.setNoDelay() will be called.
 */
void http_client_request_set_nodelay(http_client_request_t *request, bool nodelay);

/**
 * request.setSocketKeepAlive([enable][, initialDelay])#
 * enable <boolean>
 * initialDelay <number>
 * Once a socket is assigned to this request and is connected socket.setKeepAlive() will be called.
 */
void http_client_request_set_socket_keep_alive(http_client_request_t *request, bool enable, int initial_delay);

/**
 * request.setTimeout(timeout[, callback])
 * timeout <number> Milliseconds before a request times out.
 * callback <Function> Optional function to be called when a timeout occurs. Same as binding to the 'timeout' event.
 * Returns: <http.ClientRequest>
 * Once a socket is assigned to this request and is connected socket.setTimeout() will be called.
 */
void http_client_request_set_timeout(http_client_request_t *request, int timeout, http_client_request_timeout_cb cb);

/**
 * request.write(chunk[, encoding][, callback])
 * chunk <string> | <Buffer>
 * encoding <string>
 * callback <Function>
 * Returns: <boolean>
 * Sends a chunk of the body. By calling this method many times, a request body can be sent to a server — in that case it is suggested to use the ['Transfer-Encoding', 'chunked'] header line when creating the request.
 * The encoding argument is optional and only applies when chunk is a string. Defaults to 'utf8'.
 * The callback argument is optional and will be called when this chunk of data is flushed, but only if the chunk is non-empty.
 * Returns true if the entire data was flushed successfully to the kernel buffer. Returns false if all or part of the data was queued in user memory. 'drain' will be emitted when the buffer is free again.
 * When write function is called with empty string or buffer, it does nothing and waits for more input.
 */
typedef void(*http_client_request_write_cb)(http_client_request_t *request);
bool http_client_request_write(http_client_request_t *request, char *chunk, int len, http_client_request_write_cb cb);

//////////////////////////////////////////////////////////////////////////

/**
 * Class: http.Server
 * This class inherits from net.Server
 */

/**
 * Event: 'checkContinue'
 * request <http.IncomingMessage>
 * response <http.ServerResponse>
 * Emitted each time a request with an HTTP Expect: 100-continue is received. If this event is not listened for, the server will automatically respond with a 100 Continue as appropriate.
 * Handling this event involves calling response.writeContinue() if the client should continue to send the request body, or generating an appropriate HTTP response (e.g. 400 Bad Request) if the client should not continue to send the request body.
 * Note that when this event is emitted and handled, the 'request' event will not be emitted.
 */
typedef void(*http_server_on_check_cb)(http_server_t *server, http_incoming_message_t *request, http_server_response_t *response);
void http_server_on_check_continue(http_server_t *server, http_server_on_check_cb cb);

/**
 * Event: 'checkExpectation'
 * request <http.IncomingMessage>
 * response <http.ServerResponse>
 * Emitted each time a request with an HTTP Expect header is received, where the value is not 100-continue. If this event is not listened for, the server will automatically respond with a 417 Expectation Failed as appropriate.
 * Note that when this event is emitted and handled, the 'request' event will not be emitted.
 */
void http_server_on_check_expectation(http_server_t *server, http_server_on_check_cb cb);

/**
 * Event: 'clientError'
 * exception <Error>
 * socket <net.Socket>
 * If a client connection emits an 'error' event, it will be forwarded here. Listener of this event is responsible for closing/destroying the underlying socket. For example, one may wish to more gracefully close the socket with a custom HTTP response instead of abruptly severing the connection.
 * Default behavior is to close the socket with an HTTP '400 Bad Request' response if possible, otherwise the socket is immediately destroyed.
 * socket is the net.Socket object that the error originated from.

 * When the 'clientError' event occurs, there is no request or response object, so any HTTP response sent, including response headers and payload, must be written directly to the socket object. Care must be taken to ensure the response is a properly formatted HTTP response message.
 * err is an instance of Error with two extra columns:
 * bytesParsed: the bytes count of request packet that Node.js may have parsed correctly;
 * rawPacket: the raw packet of current request.
 */
typedef void(*http_server_on_client_error_cb)(http_server_t *server, int error, int bytes_parsed, char *raw_packet, int packet_len, net_socket_t *cocket);
void http_server_on_client_error(http_server_t *server, http_server_on_client_error_cb cb);

/**
 * Event: 'close'
 * Emitted when the server closes.
 */
typedef void(*http_server_on_close_cb)(http_server_t *server);
void http_server_on_close(http_server_t *server, http_server_on_close_cb cb);

/**
 * Event: 'connect'
 * request <http.IncomingMessage> Arguments for the HTTP request, as it is in the 'request' event
 * socket <net.Socket> Network socket between the server and client
 * head <Buffer> The first packet of the tunneling stream (may be empty)
 * Emitted each time a client requests an HTTP CONNECT method. If this event is not listened for, then clients requesting a CONNECT method will have their connections closed.
 * After this event is emitted, the request's socket will not have a 'data' event listener, meaning it will need to be bound in order to handle data sent to the server on that socket.
 */
typedef void(*http_server_on_connect_cb)(http_server_t *server, http_incoming_message_t *request, net_socket_t *socket, char *head, int len);
void http_server_on_connect(http_server_t *server, http_server_on_connect_cb cb);

/**
 * Event: 'connection'
 * socket <net.Socket>
 * This event is emitted when a new TCP stream is established. socket is typically an object of type net.Socket. Usually users will not want to access this event. In particular, the socket will not emit 'readable' events because of how the protocol parser attaches to the socket. The socket can also be accessed at request.connection.
 * This event can also be explicitly emitted by users to inject connections into the HTTP server. In that case, any Duplex stream can be passed.
 */
typedef void(*http_server_on_connection_cb)(http_server_t *server, net_socket_t *socket);
void http_server_on_connection(http_server_t *server, http_server_on_connection_cb cb);

/**
 * Event: 'request'
 * request <http.IncomingMessage>
 * response <http.ServerResponse>
 * Emitted each time there is a request. Note that there may be multiple requests per connection (in the case of HTTP Keep-Alive connections).
 */
typedef void(*http_server_on_request_cb)(http_server_t *server, http_incoming_message_t *request, http_server_response_t *response);
void http_server_on_request(http_server_t *server, http_server_on_request_cb cb);

/**
 * Event: 'upgrade'
 * request <http.IncomingMessage> Arguments for the HTTP request, as it is in the 'request' event
 * socket <net.Socket> Network socket between the server and client
 * head <Buffer> The first packet of the upgraded stream (may be empty)
 * Emitted each time a client requests an HTTP upgrade. Listening to this event is optional and clients cannot insist on a protocol change.
 * After this event is emitted, the request's socket will not have a 'data' event listener, meaning it will need to be bound in order to handle data sent to the server on that socket.
 */
typedef void(*http_server_on_upgrade_cb)(http_server_t *server, http_incoming_message_t *request, net_socket_t *socket, char *head, int len);
void http_server_on_upgrade(http_server_t *server, http_server_on_upgrade_cb cb);

/**
 * server.close([callback])
 * callback <Function>
 * Stops the server from accepting new connections. See net.Server.close().
 */
typedef void(*http_server_on_close_cb)(http_server_t *server);
void http_server_close(http_server_t *server, http_server_on_close_cb cb);

/**
 * server.listen()
 * Starts the HTTP server listening for connections. This method is identical to server.listen() from net.Server.
 */
void http_server_listen(http_server_t *server);

/**
 * server.setTimeout([msecs][, callback])
 * msecs <number> Default: 120000 (2 minutes)
 * callback <Function>
 * Returns: <http.Server>
 * Sets the timeout value for sockets, and emits a 'timeout' event on the Server object, passing the socket as an argument, if a timeout occurs.
 * If there is a 'timeout' event listener on the Server object, then it will be called with the timed-out socket as an argument.
 * By default, the Server's timeout value is 2 minutes, and sockets are destroyed automatically if they time out. However, if a callback is assigned to the Server's 'timeout' event, timeouts must be handled explicitly.
 */
typedef void(*http_server_on_timeout_cb)(http_server_t *server);
void http_server_set_timeout(http_server_t *server, int msecs, http_server_on_timeout_cb cb);

//////////////////////////////////////////////////////////////////////////

/**
 * Class: http.ServerResponse
 * This object is created internally by an HTTP server — not by the user. It is passed as the second parameter to the 'request' event.
 * The response inherits from Stream, and additionally implements the following:
 */

/**
 * Event: 'close'
 * Indicates that the underlying connection was terminated.
 */
typedef void(*http_server_response_on_close_cb)(http_server_response_t *response );
void http_server_response_on_close(http_server_response_t *response, http_server_response_on_close_cb cb);

/**
 * Event: 'finish'
 * Emitted when the response has been sent. More specifically, this event is emitted when the last segment of the response headers and body have been handed off to the operating system for transmission over the network. It does not imply that the client has received anything yet.
 */
typedef void(*http_server_response_on_finish_cb)(http_server_response_t *response );
void http_server_response_on_finish(http_server_response_t *response, http_server_response_on_finish_cb cb);

/**
 * response.addTrailers(headers)
 * headers <Object>
 * This method adds HTTP trailing headers (a header but at the end of the message) to the response.
 * Trailers will only be emitted if chunked encoding is used for the response; if it is not (e.g. if the request was HTTP/1.0), they will be silently discarded.
 * Note that HTTP requires the Trailer header to be sent in order to emit trailers, with a list of the header fields in its value. E.g.,
 * Attempting to set a header field name or value that contains invalid characters will result in a TypeError being thrown.
 */
void http_server_response_add_trailers(http_server_response_t *response, char* headers);

/**
 * response.end([data][, encoding][, callback])
 * data <string> | <Buffer>
 * encoding <string>
 * callback <Function>
 * Returns: <this>
 * This method signals to the server that all of the response headers and body have been sent; that server should consider this message complete. The method, response.end(), MUST be called on each response.
 * If data is specified, it is equivalent to calling response.write(data, encoding) followed by response.end(callback).
 * If callback is specified, it will be called when the response stream is finished.
 */
void http_server_response_end(http_server_response_t *response, char* data, int len, http_server_response_on_finish_cb cb);

/**
 * response.getHeader(name)
 * name <string>
 * Returns: <any>
 * Reads out a header that's already been queued but not sent to the client. Note that the name is case insensitive. The type of the return value depends on the arguments provided to response.setHeader().
 */
int http_server_response_get_header(http_server_response_t *response, char *name, char **value);

/**
 * response.getHeaderNames()
 * Returns: <string[]>
 * Returns an array containing the unique names of the current outgoing headers. All header names are lowercase.
 */
int http_server_response_get_header_names(http_server_response_t *response, char **names);

/**
 * response.getHeaders()
 * Returns: <Object>
 * Returns a shallow copy of the current outgoing headers. Since a shallow copy is used, array values may be mutated without additional calls to various header-related http module methods. The keys of the returned object are the header names and the values are the respective header values. All header names are lowercase.
 * The object returned by the response.getHeaders() method does not prototypically inherit from the JavaScript Object. This means that typical Object methods such as obj.toString(), obj.hasOwnProperty(), and others are not defined and will not work.
 */
char* http_server_response_get_headers(http_server_response_t *response);

/**
 * response.hasHeader(name)
 * name <string>
 * Returns: <boolean>
 * Returns true if the header identified by name is currently set in the outgoing headers. Note that the header name matching is case-insensitive.
 */
bool http_server_response_has_header(http_server_response_t *response, char *name);

/**
 * response.removeHeader(name)
 * name <string>
 * Removes a header that's queued for implicit sending.
 */
void http_server_response_remove_header(http_server_response_t *response, char *name);

/**
 * response.setHeader(name, value)
 * name <string>
 * value <any>
 * Sets a single header value for implicit headers. If this header already exists in the to-be-sent headers, its value will be replaced. Use an array of strings here to send multiple headers with the same name. Non-string values will be stored without modification. Therefore, response.getHeader() may return non-string values. However, the non-string values will be converted to strings for network transmission.
 * Attempting to set a header field name or value that contains invalid characters will result in a TypeError being thrown.
 * When headers have been set with response.setHeader(), they will be merged with any headers passed to response.writeHead(), with the headers passed to response.writeHead() given precedence.
 * If response.writeHead() method is called and this method has not been called, it will directly write the supplied header values onto the network channel without caching internally, and the response.getHeader() on the header will not yield the expected result. If progressive population of headers is desired with potential future retrieval and modification, use response.setHeader() instead of response.writeHead().
 */
void http_server_response_set_header(http_server_response_t *response, char *name, char **value, int num);

/**
 * response.setTimeout(msecs[, callback])
 * msecs <number>
 * callback <Function>
 * Returns: <http.ServerResponse>
 * Sets the Socket's timeout value to msecs. If a callback is provided, then it is added as a listener on the 'timeout' event on the response object.
 * If no 'timeout' listener is added to the request, the response, or the server, then sockets are destroyed when they time out. If a handler is assigned to the request, the response, or the server's 'timeout' events, timed out sockets must be handled explicitly.
 */
typedef void(*http_server_response_on_timeout_cb)(http_server_response_t *response);
void http_server_response_set_timeout(http_server_response_t *response, int msecs, http_server_response_on_timeout_cb cb);

/**
 * response.write(chunk[, encoding][, callback])
 * chunk <string> | <Buffer>
 * encoding <string> Default: 'utf8'
 * callback <Function>
 * Returns: <boolean>
 * If this method is called and response.writeHead() has not been called, it will switch to implicit header mode and flush the implicit headers.
 * This sends a chunk of the response body. This method may be called multiple times to provide successive parts of the body.
 * Note that in the http module, the response body is omitted when the request is a HEAD request. Similarly, the 204 and 304 responses must not include a message body.
 * chunk can be a string or a buffer. If chunk is a string, the second parameter specifies how to encode it into a byte stream. callback will be called when this chunk of data is flushed.
 * This is the raw HTTP body and has nothing to do with higher-level multi-part body encodings that may be used.
 * The first time response.write() is called, it will send the buffered header information and the first chunk of the body to the client. The second time response.write() is called, Node.js assumes data will be streamed, and sends the new data separately. That is, the response is buffered up to the first chunk of the body.
 * Returns true if the entire data was flushed successfully to the kernel buffer. Returns false if all or part of the data was queued in user memory. 'drain' will be emitted when the buffer is free again.
 */
typedef void(*http_server_response_on_write_cb)(http_server_response_t *response, int err);
bool http_server_response_write(http_server_response_t *response, char *chunk, int len, http_server_response_on_write_cb cb);

/**
 * response.writeContinue()
 Sends a HTTP/1.1 100 Continue message to the client, indicating that the request body should be sent. See the 'checkContinue' event on Server.
 */
void http_server_response_write_continue(http_server_response_t *response);

/**
 * response.writeHead(statusCode[, statusMessage][, headers])
 * statusCode <number>
 * statusMessage <string>
 * headers <Object>
 * Sends a response header to the request. The status code is a 3-digit HTTP status code, like 404. The last argument, headers, are the response headers. Optionally one can give a human-readable statusMessage as the second argument.
 * This method must only be called once on a message and it must be called before response.end() is called.
 * If response.write() or response.end() are called before calling this, the implicit/mutable headers will be calculated and call this function.
 * When headers have been set with response.setHeader(), they will be merged with any headers passed to response.writeHead(), with the headers passed to response.writeHead() given precedence.
 * If this method is called and response.setHeader() has not been called, it will directly write the supplied header values onto the network channel without caching internally, and the response.getHeader() on the header will not yield the expected result. If progressive population of headers is desired with potential future retrieval and modification, use response.setHeader() instead.
 */
void http_server_response_write_head(http_server_response_t *response, int status_code, char* status_message, char** headers); 

//////////////////////////////////////////////////////////////////////////

/**
 * Class: http.IncomingMessage
 * An IncomingMessage object is created by http.Server or http.ClientRequest and passed as the first argument to the 'request' and 'response' event respectively. It may be used to access response status, headers and data.
 * It implements the Readable Stream interface, as well as the following additional events, methods, and properties.
 */

/**
 * Event: 'aborted'
 * Emitted when the request has been aborted.
 */
typedef void(*http_incoming_message_on_aborted_cb)(http_incoming_message_t *message);
void http_incoming_message_on_aborted(http_incoming_message_t *message, http_incoming_message_on_aborted_cb cb);

/**
 * Event: 'close'
 * Indicates that the underlying connection was closed.
 */
typedef void(*http_incoming_message_on_close_cb)(http_incoming_message_t *message);
void http_incoming_message_on_close(http_incoming_message_t *message, http_incoming_message_on_close_cb cb);

/**
 * message.destroy([error])
 * error <Error>
 * Calls destroy() on the socket that received the IncomingMessage. If error is provided, an 'error' event is emitted and error is passed as an argument to any listeners on the event.
 */
void http_incoming_message_destroy(http_incoming_message_t *message, int error);

/**
 * message.setTimeout(msecs, callback)
 * msecs <number>
 * callback <Function>
 * Returns: <http.IncomingMessage>
 * Calls message.connection.setTimeout(msecs, callback).
 */
typedef void(*http_incoming_message_on_timeout_cb)(http_incoming_message_t *message);
void http_incoming_message_set_timeout(http_incoming_message_t *message, int msecs, http_incoming_message_on_timeout_cb cb);

/**
 * http.createServer([options][, requestListener])
 * options <Object>
 * * IncomingMessage <http.IncomingMessage> Specifies the IncomingMessage class to be used. Useful for extending the original IncomingMessage. Default: IncomingMessage.
 * * ServerResponse <http.ServerResponse> Specifies the ServerResponse class to be used. Useful for extending the original ServerResponse. Default: ServerResponse.
 * requestListener <Function>
 * Returns: <http.Server>
 * Returns a new instance of http.Server.
 * The requestListener is a function which is automatically added to the 'request' event.
 */
http_server_t* http_create_server(uv_node_t* h, http_server_on_request_cb cb);

/**
 * http.get(options[, callback])
 * http.get(url[, options][, callback])
 * url <string> | <URL>
 * options <Object> Accepts the same options as http.request(), with the method always set to GET. Properties that are inherited from the prototype are ignored.
 * callback <Function>
 * Returns: <http.ClientRequest>
 * Since most requests are GET requests without bodies, Node.js provides this convenience method. The only difference between this method and http.request() is that it sets the method to GET and calls req.end() automatically. Note that the callback must take care to consume the response data for reasons stated in http.ClientRequest section.
 * The callback is invoked with a single argument that is an instance of http.IncomingMessage.
 */
http_client_request_t* http_get(uv_node_t* h,char *url, char **options, char **headers, http_agent_t *agent, http_client_request_response_cb cb);

/**
 * http.request(options[, callback])
 * http.request(url[, options][, callback])
 * url 字符串，包含protocol、host、port、path
 * options 二维数组 {"key","value","key","value",NULL}
 * * protocol <string> Protocol to use. Default: 'http:'.
 * * host <string> A domain name or IP address of the server to issue the request to. Default: 'localhost'.
 * * hostname <string> Alias for host. To support url.parse(), hostname will be used if both host and hostname are specified.
 * * family <number> IP address family to use when resolving host or hostname. Valid values are 4 or 6. When unspecified, both IP v4 and v6 will be used.
 * * port <number> Port of remote server. Default: 80.
 * * localAddress <string> Local interface to bind for network connections.
 * * socketPath <string> Unix Domain Socket (cannot be used if one of host or port is specified, those specify a TCP Socket).
 * * method <string> A string specifying the HTTP request method. Default: 'GET'.
 * * path <string> Request path. Should include query string if any. E.G. '/index.html?page=12'. An exception is thrown when the request path contains illegal characters. Currently, only spaces are rejected but that may change in the future. Default: '/'.
 * * headers <Object> An object containing request headers.
 * * auth <string> Basic authentication i.e. 'user:password' to compute an Authorization header.
 * * createConnection <Function> A function that produces a socket/stream to use for the request when the agent option is not used. This can be used to avoid creating a custom Agent class just to override the default createConnection function. See agent.createConnection() for more details. Any Duplex stream is a valid return value.
 * * timeout <number>: A number specifying the socket timeout in milliseconds. This will set the timeout before the socket is connected.
 * * setHost <boolean>: Specifies whether or not to automatically add the Host header. Defaults to true.
 * agent <http.Agent> | <boolean> Controls Agent behavior. Possible values:
 * * 0 (default): use http.globalAgent for this host and port.
 * * Agent object: explicitly use the passed in Agent.
 * * -1: causes a new Agent with default values to be used.
 * callback <Function>
 * Returns: <http.ClientRequest>
 * Node.js maintains several connections per server to make HTTP requests. This function allows one to transparently issue requests.
 * url can be a string or a URL object. If url is a string, it is automatically parsed with new URL(). If it is a URL object, it will be automatically converted to an ordinary options object.
 * If both url and options are specified, the objects are merged, with the options properties taking precedence.
 * The optional callback parameter will be added as a one-time listener for the 'response' event.
 * http.request() returns an instance of the http.ClientRequest class. The ClientRequest instance is a writable stream. If one needs to upload a file with a POST request, then write to the ClientRequest object.
 */
http_client_request_t* http_request(uv_node_t* h, char *url, char **options, char **headers, http_agent_t *agent ,http_client_request_response_cb cb);

#ifdef __cplusplus
       }
#endif

#endif