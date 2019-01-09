#ifndef _UV_HTTP_REQUEST_
#define _UV_HTTP_REQUEST_ 

#ifdef __cplusplus
extern "C" {
#endif

#include "public_def.h"
#include "agent.h"

typedef struct _socket_ socket_t;

typedef enum _http_event_ {
    http_event_abort = 0,
    http_event_connect,
    http_event_information,
    http_event_response,
    http_event_socket,
    http_event_timeout,
    http_event_upgrade,
    http_event_number
}http_event_t;

typedef struct _http_request_options_ {
    char*    protocol;     //Protocol to use. Default: 'http:'.
    char*    host;         //A domain name or IP address of the server to issue the request to. Default: 'localhost'.
    char*    hostname;     //Alias for host. To support url.parse(), hostname will be used if both host and hostname are specified.
    int      family;       //IP address family to use when resolving host or hostname. Valid values are 4 or 6. When unspecified, both IP v4 and v6 will be used.
    int      port;         //Port of remote server. Default: 80.
    char*    localAddress; //Local interface to bind for network connections.
    char*    socketPath;   //Unix Domain Socket (cannot be used if one of host or port is specified, those specify a TCP Socket).
    HTTP_METHOD    method; //A string specifying the HTTP request method. Default: 'GET'.
    char*    path;         //Request path. Should include query string if any. E.G. '/index.html?page=12'. An exception is thrown when the request path contains illegal characters. Currently, only spaces are rejected but that may change in the future. Default: '/'.
    char**   headers;      //An object containing request headers.
    char*    auth;         //Basic authentication i.e. 'user:password' to compute an Authorization header.
    http_agent_t *agent;   //Controls Agent behavior. NULL(default): use http.globalAgent for this host and port. -1: causes a new Agent with default values to be used.
    agent_create_connection createConnection; //A function that produces a socket/stream to use for the request when the agent option is not used. This can be used to avoid creating a custom Agent class just to override the default createConnection function. See agent.createConnection() for more details. Any Duplex stream is a valid return value.
    int      timeout;      //A number specifying the socket timeout in milliseconds. This will set the timeout before the socket is connected.
    int      setHost;      //Specifies whether or not to automatically add the Host header. Defaults to true.
}http_request_options_t;

typedef struct _http_client_request_ {
    int aborted;  //The request.aborted property will be true if the request has been aborted.
    int finished; //The request.finished property will be true if request.end() has been called. request.end() will automatically be called if the request was initiated via http.get().
    int maxHeadersCount; //Limits maximum response headers count. If set to 0, no limit will be applied.
    socket_t* socket; //Reference to the underlying socket. Usually users will not want to access this property. In particular, the socket will not emit 'readable' events because of how the protocol parser attaches to the socket. The socket may also be accessed via request.connection.
}http_client_request_t;


extern http_request_options_t http_request_option();

/**
 * @param options: see http_request_options_t
 * @param cb: can be null and use http_request_on_response later
 */
typedef void (*on_request_cb)(response_t res);
typedef void (*on_response_cb)(response_t* res);
extern http_client_request_t* http_request(http_t* h, http_request_options_t *options, on_response_cb cb);
//extern http_client_request_t* http_request(http_t* h, char* url, http_request_options_t *options, on_request_cb cb);

/**
 * Event: 'abort'
 * Emitted when the request has been aborted by the client. This event is only emitted on the first call to abort().
 */
typedef void (*on_abort_cb)(http_client_request_t* req);
extern void http_request_on_abort(http_client_request_t* req, on_abort_cb cb);

/**
 * Event: 'continue'
 * Emitted when the server sends a '100 Continue' HTTP response, usually because the request contained 'Expect: 100-continue'. This is an instruction that the client should send the request body.
 */
typedef void (*on_continue_cb)(http_client_request_t* req);
extern void http_request_on_continue(http_client_request_t* req, on_continue_cb cb);

/**
 * Event: 'information'
 * Emitted when the server sends a 1xx response (excluding 101 Upgrade). This event is emitted with a callback containing an object with a status code.
 */
typedef void (*on_information_cb)(response_t* res);
extern void http_request_on_information(http_client_request_t* req, on_information_cb cb);

/**
 * Event: 'response'
 * response <http.IncomingMessage>
 * Emitted when a response is received to this request. This event is emitted only once.
 */
extern void http_request_on_response(http_client_request_t* req, on_response_cb cb);

/**
 * Event: 'socket'
 * socket <net.Socket>
 * Emitted after a socket is assigned to this request.
 */
typedef void (*on_socket_cb)(socket_t* socket);
extern void http_request_on_socket(http_client_request_t* req, on_socket_cb cb);

/**
 * Event: 'timeout'
 * Emitted when the underlying socket times out from inactivity. This only notifies that the socket has been idle. The request must be aborted manually.
 */
typedef void (on_timeout_cb)(http_client_request_t* req);
extern void http_request_on_timeout(http_client_request_t* req, on_timeout_cb cb);

/**
 * Marks the request as aborting. Calling this will cause remaining data in the response to be dropped and the socket to be destroyed.
 */
extern void http_request_abort(http_client_request_t* req);

/**
 * Flush the request headers.
 * For efficiency reasons, Node.js normally buffers the request headers until request.end() is called or the first chunk of request data is written. It then tries to pack the request headers and data into a single TCP packet.
 * That's usually desired (it saves a TCP round-trip), but not when the first data is not sent until possibly much later. request.flushHeaders() bypasses the optimization and kickstarts the request.
 */
extern void http_request_flush_headers(http_client_request_t* req);

/**
 * Reads out a header on the request. Note that the name is case insensitive. The type of the return value depends on the arguments provided to request.setHeader().
 */
extern const char* http_request_get_header(http_client_request_t* req, const char* name);

/**
 * Removes a header that's already defined into headers object.
 */
extern void http_request_remove_header(http_client_request_t* req, const char* name);

/**
 * Sets a single header value for headers object. If this header already exists in the to-be-sent headers, its value will be replaced. Use an array of strings here to send multiple headers with the same name. Non-string values will be stored without modification. Therefore, request.getHeader() may return non-string values. However, the non-string values will be converted to strings for network transmission.
 * name <string>
 * value <any>
 */
extern void http_request_set_header(http_client_request_t* req, char* name, char* value);

/**
 * @param timeout: Milliseconds before a request times out.
 * @param cb: Optional function to be called when a timeout occurs. Same as binding to the 'timeout' event.
 * @note Once a socket is assigned to this request and is connected socket.setTimeout() will be called.
 */
extern void http_request_set_timeout(http_client_request_t* req, int timeout, on_timeout_cb cb);

/**
 * Sends a chunk of the body. By calling this method many times, a request body can be sent to a server ¡ª in that case it is suggested to use the ['Transfer-Encoding', 'chunked'] header line when creating the request.
 * @param encoding: is optional and only applies when chunk is a string. Defaults to 'utf8'.
 * @param cb: is optional and will be called when this chunk of data is flushed, but only if the chunk is non-empty.
 * @return 0 if the entire data was flushed successfully to the kernel buffer. Returns -1 if all or part of the data was queued in user memory. 'drain' will be emitted when the buffer is free again.
 * @note When write function is called with empty string or buffer, it does nothing and waits for more input.
 */
typedef void (*on_request_write)(http_client_request_t* req);
extern int http_request_write(http_client_request_t* req, char* chunk, int len, char* encoding, on_request_write cb);

/**
 * Finishes sending the request. If any parts of the body are unsent, it will flush them to the stream. If the request is chunked, this will send the terminating '0\r\n\r\n'.
 * data <string> | <Buffer>
 * encoding <string>
 * callback <Function>
 * If data is specified, it is equivalent to calling request.write(data, encoding) followed by request.end(callback).
 * If callback is specified, it will be called when the request stream is finished.
 */
typedef void (*on_request_end)(http_client_request_t* req);
extern void http_request_end(http_client_request_t* req, char* data, int len, char* encoding, on_request_end cb);

#ifdef __cplusplus
}
#endif

#endif