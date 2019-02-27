#include "public.h"
#include "private.h"
#include "http.h"
#include "cstl_easy.h"

/**
 * ┌───────────────────────────────────────────┐
 * │                                    href                                              │
 * ├─────┬┬──────┬─────────┬──────────────┬────┤
 * │ protocol ││   auth     │       host       │          path              │  hash  │
 * │          ││            ├─────┬───┼─────┬────────┤        │
 * │          ││            │ hostname │ port │ pathname │     search     │        │
 * │          ││            │          │      │          ├┬───────┤        │
 * │          ││            │          │      │          ││    query     │        │
 *  "  http:   // user:pass    @  host.com :  8080  /p/a/t/h    ?  query=string   #  hash  "
 * │          ││            │          │      │          ││              │        │
 * └─────┴┴──────┴─────┴───┴─────┴┴───────┴────┘
 */
typedef struct _http_url_ {
    string_t                          *protocol;        //Protocol to use. Default: 'http:'.
    bool                              slashes;
    string_t                          *auth;            //Basic authentication i.e. 'user:password' to compute an Authorization header.
    string_t                          *host;            //A domain name or IP address of the server to issue the request to. Default: 'localhost'.
    string_t                          *hostname;
    int                               port;
    string_t                          *path;             //Request path. Should include query string if any. E.G. '/index.html?page=12'. An exception is thrown when the request path contains illegal characters. Currently, only spaces are rejected but that may change in the future. Default: '/'.
    string_t                          *pathname;
    string_t                          *search;
    string_t                          *hash;
    string_t                          *href;
}http_url_t;

typedef struct _http_agent_ {
    list_t         *sockets;       //An object which contains arrays of sockets currently in use by the agent. Do not modify.
    list_t         *free_sockets;  //An object which contains arrays of sockets currently awaiting use by the agent when keepAlive is enabled. Do not modify.
    list_t         *requests;      //An object which contains queues of requests that have not yet been assigned to sockets. Do not modify.
    bool           keepAlive;      //Keep sockets around even when there are no outstanding requests, so they can be used for future requests without having to reestablish a TCP connection. Default: false.
    int            keepAliveMsecs; //When using the keepAlive option, specifies the initial delay for TCP Keep-Alive packets. Ignored when the keepAlive option is false or undefined. Default: 1000.
    int            maxFreeSockets; //By default set to 256. For agents with keepAlive enabled, this sets the maximum number of sockets that will be left open in the free state.
    int            maxSockets;     //By default set to Infinity. Determines how many concurrent sockets the agent can have open per origin. Origin is the returned value of agent.getName().
    int            timeout;        //Socket timeout in milliseconds. This will set the timeout after the socket is connected.
    uv_node_t*        h;
}http_agent_t;

typedef struct _http_client_request_ {
    uv_node_t                         *h;
    net_socket_t                      *socket;
    //以下几项记录request的状态
    bool                              aborted;          //The request.aborted property will be true if the request has been aborted.
    bool                              finished;         //The request.finished property will be true if request.end() has been called. request.end() will automatically be called if the request was initiated via http.get().
    //以下几项是用户配置参数
    string_t                          *protocol;        //Protocol to use. Default: 'http:'.
    string_t                          *host;            //A domain name or IP address of the server to issue the request to. Default: 'localhost'.
    string_t                          *hostname;        //Alias for host. To support url.parse(), hostname will be used if both host and hostname are specified.
    int                               port;             //Port of remote server. Default: 80.
    int                               family;           //IP address family to use when resolving host or hostname. Valid values are 4 or 6. When unspecified, both IP v4 and v6 will be used.
    string_t                          *localAddress;     //Local interface to bind for network connections.
    string_t                          *socketPath;       //Unix Domain Socket (cannot be used if one of host or port is specified, those specify a TCP Socket).
    string_t                          *method;           //A string specifying the HTTP request method. Default: 'GET'.
    string_t                          *path;             //Request path. Should include query string if any. E.G. '/index.html?page=12'. An exception is thrown when the request path contains illegal characters. Currently, only spaces are rejected but that may change in the future. Default: '/'.
    list_t                            *headers;          //An object containing request headers.
    string_t                          *auth;             //Basic authentication i.e. 'user:password' to compute an Authorization header.
    http_agent_t                      *agent;            // -1 (default): use http.globalAgent for this host and port.; 0: causes a new Agent with default values to be used.; Agent object: explicitly use the passed in Agent.
    int                                timeout;          //A number specifying the socket timeout in milliseconds. This will set the timeout before the socket is connected.
    bool                               setHost;          //Specifies whether or not to automatically add the Host header. Defaults to true.
    int                                max_headers_count; //Default: 2000 Limits maximum response headers count. If set to 0, no limit will be applied.
    bool                               nodelay;
    bool                               keepAlive; 
    int                                keepAliveMsecs; 
    //以下为回调函数
    http_client_request_abort_cb       on_abort;
    http_client_request_connect_cb     on_connect;
    http_client_request_continue_cb    on_continue;
    http_client_request_information_cb on_information;
    http_client_request_response_cb    on_response;
    http_client_request_socket_cb      on_socket;
    http_client_request_timeout_cb     on_timeout;
    http_client_request_upgrade_cb     on_upgrade;
    http_client_request_end_cb         on_end;
    //内部数据保存
    string_t                           *parse_headers;
    bool                               send_header;
}http_client_request_t;

typedef struct _http_incoming_message_ {
    net_socket_t *socket;
    bool      aborted;    //The message.aborted property will be true if the request has been aborted.
    // The message.complete property will be true if a complete HTTP message has been received and successfully parsed.
    //This property is particularly useful as a means of determining if a client or server fully transmitted a message before a connection was terminated:
    bool      complete;
    map_t     headers;    //The request/response headers object.Key-value pairs of header names and values. Header names are lower-cased.
    //In case of server request, the HTTP version sent by the client. In the case of client response, the HTTP version of the connected-to server. Probably either '1.1' or '1.0'.
    //Also message.httpVersionMajor is the first integer and message.httpVersionMinor is the second.
    char      *http_version;
    char      *method;    //Only valid for request obtained from http.Server. The request method as a string. Read only. Examples: 'GET', 'DELETE'.
    //The raw request/response headers list exactly as they were received.
    //Note that the keys and values are in the same list. It is not a list of tuples. So, the even-numbered offsets are key values, and the odd-numbered offsets are the associated values.
    //Header names are not lowercased, and duplicates are not merged.
    string_t  *raw_headers;
    string_t  *raw_trailers;    //The raw request/response trailer keys and values exactly as they were received. Only populated at the 'end' event.
    int       statusCode;  //Only valid for response obtained from http.ClientRequest.
    char      *status_message; //Only valid for response obtained from http.ClientRequest.
    list_t    *trailers;    //The request/response trailers object. Only populated at the 'end' event.
    char      *url;    //Only valid for request obtained from http.Server.

    http_incoming_message_on_aborted_cb  on_aborted;
    http_incoming_message_on_close_cb on_close;
    http_incoming_message_on_timeout_cb on_timeout;
}http_incoming_message_t;

typedef struct _http_server_response_ {
    net_socket_t *socket;
    bool      finished;    //Boolean value that indicates whether the response has completed. Starts as false. After response.end() executes, the value will be true.
    bool      headers_sent;//Boolean (read-only). True if headers were sent, false otherwise.
    int       statusCode;  //When using implicit headers (not calling response.writeHead() explicitly), this property controls the status code that will be sent to the client when the headers get flushed.
    char      *status_message; //When using implicit headers (not calling response.writeHead() explicitly), this property controls the status message that will be sent to the client when the headers get flushed. If this is left as undefined then the standard message for the status code will be used.

    http_server_response_on_close_cb on_close;
    http_server_response_on_finish_cb on_finish;
    http_server_response_on_timeout_cb on_timeout;
    http_server_response_on_write_cb on_write;
}http_server_response_t;

typedef struct _http_server_ {
    /**
     * Default: 40000 
     * Limit the amount of time the parser will wait to receive the complete HTTP headers.
     * In case of inactivity, the rules defined in server.timeout apply. 
     * However, that inactivity based timeout would still allow the connection to be kept open if the headers are being sent very slowly (by default, up to a byte per 2 minutes). 
     * In order to prevent this, whenever header data arrives an additional check is made that more than server.headersTimeout milliseconds has not passed since the connection was established. 
     * If the check fails, a 'timeout' event is emitted on the server object, and (by default) the socket is destroyed. See server.timeout for more information on how timeout behavior can be customized.
     */
    int      headers_timeout;
    bool     listening;    //Indicates whether or not the server is listening for connections.
    int      max_headers_count; //Default: 2000 Limits maximum incoming headers count. If set to 0, no limit will be applied.
    /**
     * Timeout in milliseconds. Default: 120000 (2 minutes).
     * The number of milliseconds of inactivity before a socket is presumed to have timed out.
     * A value of 0 will disable the timeout behavior on incoming connections.
     * The socket timeout logic is set up on connection, so changing this value only affects new connections to the server, not any existing connections.
     */
    int      timeout;
    /**
     * server.keepAliveTimeout
     * <number> Timeout in milliseconds. Default: 5000 (5 seconds).
     * The number of milliseconds of inactivity a server needs to wait for additional incoming data, after it has finished writing the last response, before a socket will be destroyed. If the server receives new data before the keep-alive timeout has fired, it will reset the regular inactivity timeout, i.e., server.timeout.
     * A value of 0 will disable the keep-alive timeout behavior on incoming connections. A value of 0 makes the http server behave similarly to Node.js versions prior to 8.0.0, which did not have a keep-alive timeout.
     * The socket timeout logic is set up on connection, so changing this value only affects new connections to the server, not any existing connections.
     */
    int keep_alive_timeout;

    http_server_on_check_cb on_check_continue;
    http_server_on_check_cb on_check_expectation;
    http_server_on_client_error_cb on_client_error;
    http_server_on_close_cb on_close;
    http_server_on_connect_cb on_connect;
    http_server_on_connection_cb on_connection;
    http_server_on_request_cb on_request;
    http_server_on_timeout_cb on_timeout;
}http_server_t;

/**
 * 解析url字符串到结构体
 * url 地址字符串
 * slashesDenoteHost 默认false。url为协议开头时无效。
 * url以//开头，true://和第一个/之间的内容为host; false://及后面所有内容为path
 */
http_url_t* http_url_parse(char* url, bool slashesDenoteHost) {
    SAFE_MALLOC(http_url_t, ret);
    ret->href = create_string();
    string_init_cstr(ret->href, url);
    if(url[0]=='/'&&url[1]=='//'){
        // 以//开头，使用默认协议
        char *tmp = url+2;
        if(slashesDenoteHost) {
            //url包含host
            char *p = strstr(tmp, "/");
            ret->host = create_string();
            if(p){
                string_init_subcstr(ret->host, tmp, p-tmp);
                tmp = p;
                p = strstr(tmp, "#");
                ret->path = create_string();
                if(p) {
                    string_init_subcstr(ret->path, tmp, p-tmp);
                    tmp = p+1;
                    if(strlen(tmp) > 0) {
                        ret->hash = create_string();
                        string_init_cstr(ret->hash, tmp);
                    }
                } else {
                    string_init_cstr(ret->path, tmp);
                }
            } else {
                string_init_cstr(ret->host, tmp);
            }
        } else {
            //url以path开头
            char *p = strstr(tmp, "#");
            if(p){
                ret->path = create_string();
                string_init_subcstr(ret->path, url, p-url);
                ret->hash = create_string();
                string_init_cstr(ret->path, p+1);
            } else {
                ret->path = create_string();
                string_init_cstr(ret->path, url);
            }
        }
    } else {
        //url以协议开头
        char *p = strstr(url, "://");
        if(p) {
            char *tmp = p+3;
            ret->protocol = create_string();
            string_init_subcstr(ret->protocol, url, p-url);
            p = strstr(tmp, "/");
            ret->host = create_string();
            if(p){
                string_init_subcstr(ret->host, tmp, p-tmp);
                tmp = p;
                p = strstr(tmp, "#");
                ret->path = create_string();
                if(p) {
                    string_init_subcstr(ret->path, tmp, p-tmp);
                    tmp = p+1;
                    if(strlen(tmp) > 0) {
                        ret->hash = create_string();
                        string_init_cstr(ret->hash, tmp);
                    }
                } else {
                    string_init_cstr(ret->path, tmp);
                }
            } else {
                string_init_cstr(ret->host, tmp);
            }
        } else {
            // 非法的url
            http_url_destory(ret);
            return NULL;
        }
    }
    // host分割为auth,hostname,port
    if(ret->host) {
        size_t pos = string_find_char(ret->host, '@', 0);
        if(pos != NPOS) {
            string_t *tmp;
            ret->auth = create_string();
            string_init_copy_substring(ret->auth, ret->host, 0, pos);
            tmp = string_substr(ret->host, pos+1, string_size(ret->host)-pos-1);
            string_destroy(ret->host);
            ret->host = tmp;
        }
        pos = string_find_char(ret->host, ':', 0);
        ret->hostname = create_string();
        if(pos != NPOS) {
            string_init_copy_substring(ret->hostname, ret->host, 0, pos);
            if(string_size(ret->host) > pos+1) {
                string_t *tmp = string_substr(ret->host, pos+1, string_size(ret->host)-pos-1);
                ret->port = atoi(string_c_str(tmp));
                string_destroy(tmp);
            } else {
                string_init_copy(ret->hostname, ret->host);
            }
        }
    }
    // path分割为pathname，search
    if(ret->path) {
        size_t pos = string_find_char(ret->path, '?', 0);
        ret->pathname = create_string();
        if (pos != NPOS) {
            string_init_copy_substring(ret->pathname, ret->path, 0, pos);
            ret->search = create_string();
            string_init_copy_substring(ret->pathname, ret->path, pos, string_size(ret->path));
        } else {
            string_init_copy(ret->pathname, ret->path);
        }
    }
    return ret;
}

void http_url_destory(http_url_t *url) {
    if(url->protocol)
        string_destroy(url->protocol);
    if(url->auth)
        string_destroy(url->auth);
    if(url->host)
        string_destroy(url->host);
    if(url->hostname)
        string_destroy(url->hostname);
    if(url->path)
        string_destroy(url->path);
    if(url->pathname)
        string_destroy(url->pathname);
    if(url->search)
        string_destroy(url->search);
    if(url->hash)
        string_destroy(url->hash);
    if(url->href)
        string_destroy(url->href);
    free(url);
}

http_agent_t* http_create_agent(uv_node_t *h, char **options) {
    SAFE_MALLOC(http_agent_t, ret);
    ret->h = h;
    ret->keepAlive      = false;
    ret->keepAliveMsecs = 1000;
    ret->maxSockets     = 0;
    ret->maxFreeSockets = 256;
    ret->timeout        = 0;
    ret->sockets        = create_list(void*);
    ret->free_sockets   = create_list(void*);
    ret->requests       = create_list(void*);
    list_init(ret->sockets);
    list_init(ret->free_sockets);
    list_init(ret->requests);

    if(options){
        char *key = options[0], *value = options[1];
        int i = 0;
        for(;key&&value;key = options[i], value = options[i+1]){
            if(!strcasecmp(key,"keepAlive")){
                if(!strcasecmp(value, "true"))
                    ret->keepAlive = true;
            } else if(!strcasecmp(key,"keepAliveMsecs")){
                ret->keepAliveMsecs = atoi(value);
            } else if(!strcasecmp(key,"maxSockets")){
                ret->maxSockets = atoi(value);
            } else if(!strcasecmp(key,"maxFreeSockets")){
                ret->maxFreeSockets = atoi(value);
            } else if(!strcasecmp(key,"timeout")){
                ret->timeout = atoi(value);
            } 
            i+=2;
        }
    }
    return ret;
}

net_socket_t* http_agent_create_connection(http_agent_t* agent, net_socket_options_t *conf, net_socket_connect_options_t *options, on_socket_event cb /*= NULL*/) {
    net_socket_t* skt = net_create_socket(agent->h, conf);
    list_push_back(agent->sockets, skt);
    net_socket_connect_options(skt, options, cb);
    return skt;
}

bool http_agent_keep_socket_alive(http_agent_t* agent, net_socket_t* socket) {
    net_socket_set_keep_alive(socket, true, agent->keepAliveMsecs);
    return true;
}

void http_agent_reuse_socket(http_agent_t* agent, net_socket_t *socket, http_client_request_t *request) {
    net_socket_t *tmp;
    pair_t *use;
    LIST_FOR_BEGIN(agent->free_sockets, pair_t*, p) {
        tmp = *(net_socket_t**)pair_second(p);
        if(tmp == socket) {
            use = p;
            list_erase(agent->free_sockets, it);
            break;
        }
    }LIST_FOR_END;

    list_push_back(agent->sockets, use);

    request->socket = socket;
}

void http_agent_destory(http_agent_t* agent) {
    list_destroy(agent->sockets);
    list_destroy(agent->free_sockets);
    list_destroy(agent->requests);
    SAFE_FREE(agent);
}

char* http_agent_get_name(char* buff, char* host, int port, char* local_address, int family) {
    sprintf(buff,"%s:%d:%s:%d", host, port, local_address, family);
    return buff;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

static void http_client_request_parse_headers(http_client_request_t *request) {
    if(request->parse_headers) {
        string_clear(request->parse_headers);
    } else {
        request->parse_headers = create_string();
        string_init(request->parse_headers);
    }
    string_append(request->parse_headers, request->method);
    string_append_char(request->parse_headers, 1, ' ');
    string_append(request->parse_headers, request->path);
    string_append_cstr(request->parse_headers, " HTTP/1.1\r\n");
    if(request->setHost) {
        string_append_cstr(request->parse_headers, "HOST: ");
        string_append(request->parse_headers, request->host);
        string_append_cstr(request->parse_headers, "\r\n");
    }
    LIST_FOR_BEGIN(request->headers, pair_t*, p){
        string_t *first = *(string_t**)pair_first(p);
        string_t *second = *(string_t**)pair_second(p);
        string_append(request->parse_headers, first);
        string_append_cstr(request->parse_headers, ": ");
        string_append(request->parse_headers, request->path);
        string_append_cstr(request->parse_headers, "\r\n");
    }LIST_FOR_END;
    string_append_cstr(request->parse_headers, "\r\n");
}

void http_client_request_on_abort(http_client_request_t *request, http_client_request_abort_cb cb) {
    request->on_abort = cb;
}

void http_client_request_on_connect(http_client_request_t *request, http_client_request_connect_cb cb) {
    request->on_connect = cb;
}

void http_client_request_on_continue(http_client_request_t *request, http_client_request_continue_cb cb) {
    request->on_continue = cb;
}

void http_client_request_on_information(http_client_request_t *request, http_client_request_information_cb cb) {
    request->on_information = cb;
}

void http_client_request_on_response(http_client_request_t *request, http_client_request_response_cb cb) {
    request->on_response = cb;
}

void http_client_request_on_socket(http_client_request_t *request, http_client_request_socket_cb cb) {
    request->on_socket = cb;
}

void http_client_request_on_timeout(http_client_request_t *request, http_client_request_timeout_cb cb) {
    request->on_timeout = cb;
}

void http_client_request_on_upgrade(http_client_request_t *request, http_client_request_upgrade_cb cb) {
    request->on_upgrade = cb;
}

void http_client_request_abort(http_client_request_t *request) {
    if(request->on_abort)
        (request->on_abort)(request, 0);
}

void http_client_request_end(http_client_request_t *request, char* data, int len, http_client_request_end_cb cb) {
    request->on_end = cb;
    if(!request->send_header){
        request->send_header = true;
        http_client_request_parse_headers(request);
    }
}

void http_client_request_flush_headers(http_client_request_t *request) {
    if(!request->send_header){
        request->send_header = true;
        http_client_request_parse_headers(request);
    }
}

int http_client_request_get_header(http_client_request_t *request, char *name, char **value) {
    int ret = 0;
    string_t *tmp = create_string();
    string_init(tmp);
    LIST_FOR_BEGIN(request->headers, pair_t*, p) {
        string_t *first = *(string_t**)pair_first(p);
        if(!strcasecmp(string_c_str(first), name)) {
            string_t *second = *(string_t**)pair_second(p);
            string_append(tmp, second);
            string_append_char(tmp, 1, ',');
        }
    }LIST_FOR_END
    ret = string_size(tmp);
    if(ret == 0)
        *value = NULL;
    else {
        *value = (char*)malloc(ret);
        strncpy(*value, string_c_str(tmp), ret);
        (*value)[ret] = 0;
    }
    string_destroy(tmp);
    return ret;
}

void http_client_request_remove_header(http_client_request_t *request, char *name) {
    LIST_FOR_BEGIN_SAFE(request->headers, pair_t*, p, tmp){
        string_t *first = *(string_t**)pair_first(p);
        string_t *second = *(string_t**)pair_second(p);
        string_destroy(first);
        string_destroy(second);
        pair_destroy(p);
        list_erase(request->headers, tmp);
    }LIST_FOR_END
}

void http_client_request_set_header(http_client_request_t *request, char *name, char *value) {
    string_t *first = create_string();
    string_t *second = create_string();
    pair_t *p = create_pair(void*, void*);
    string_init_cstr(first, name);
    string_init_cstr(second, value);
    pair_init_elem(p, first, second);
    http_client_request_remove_header(request, name);
    list_push_back(request->headers, p);
}

void http_client_request_set_header2(http_client_request_t *request, char *name, char **values) {
    int i = 0;
    char* value = values[i];
    http_client_request_remove_header(request, name);
    for(;value;){
        string_t *first = create_string();
        string_t *second = create_string();
        pair_t *p = create_pair(void*, void*);
        string_init_cstr(first, name);
        string_init_cstr(second, value);
        pair_init_elem(p, first, second);
        list_push_back(request->headers, p);
    }
}

void http_client_request_set_nodelay(http_client_request_t *request, bool nodelay) {
    request->nodelay = nodelay;
}

void http_client_request_set_socket_keep_alive(http_client_request_t *request, bool enable, int initial_delay) {
    request->keepAlive = enable;
    request->keepAliveMsecs = initial_delay;
}

void http_client_request_set_timeout(http_client_request_t *request, int timeout, http_client_request_timeout_cb cb) {
    request->timeout = timeout;
    request->on_timeout = cb;
}

bool http_client_request_write(http_client_request_t *request, char *chunk, int len, http_client_request_write_cb cb) {
    if(!request->send_header){
        request->send_header = true;
        http_client_request_parse_headers(request);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////

void http_server_on_check_continue(http_server_t *server, http_server_on_check_cb cb) {

}

void http_server_on_check_expectation(http_server_t *server, http_server_on_check_cb cb) {

}

void http_server_on_client_error(http_server_t *server, http_server_on_client_error_cb cb) {

}

void http_server_on_close(http_server_t *server, http_server_on_close_cb cb) {

}

void http_server_on_connect(http_server_t *server, http_server_on_connect_cb cb) {

}

void http_server_on_connection(http_server_t *server, http_server_on_connection_cb cb) {

}

void http_server_on_request(http_server_t *server, http_server_on_request_cb cb) {

}

void http_server_on_upgrade(http_server_t *server, http_server_on_upgrade_cb cb) {

}

void http_server_close(http_server_t *server, http_server_on_close_cb cb) {

}

void http_server_listen(http_server_t *server) {

}

void http_server_set_timeout(http_server_t *server, int msecs, http_server_on_timeout_cb cb) {

}

/////////////////////////////////////////////////////////////////////////////////////////////////

void http_server_response_on_close(http_server_response_t *response, http_server_response_on_close_cb cb) {

}

void http_server_response_on_finish(http_server_response_t *response, http_server_response_on_finish_cb cb) {

}

void http_server_response_add_trailers(http_server_response_t *response, char* headers) {

}

void http_server_response_end(http_server_response_t *response, char* data, int len, http_server_response_on_finish_cb cb) {

}

int http_server_response_get_header(http_server_response_t *response, char *name, char **value) {

}

int http_server_response_get_header_names(http_server_response_t *response, char **names) {

}

char* http_server_response_get_headers(http_server_response_t *response) {

}

bool http_server_response_has_header(http_server_response_t *response, char *name) {

}

void http_server_response_remove_header(http_server_response_t *response, char *name) {

}

void http_server_response_set_header(http_server_response_t *response, char *name, char **value, int num) {

}

void http_server_response_set_timeout(http_server_response_t *response, int msecs, http_server_response_on_timeout_cb cb) {

}

bool http_server_response_write(http_server_response_t *response, char *chunk, int len, http_server_response_on_write_cb cb) {

}

void http_server_response_write_continue(http_server_response_t *response) {

}

void http_server_response_write_head(http_server_response_t *response, int status_code, char* status_message, char** headers) {

}

//////////////////////////////////////////////////////////////////////////

void http_incoming_message_on_aborted(http_incoming_message_t *message, http_incoming_message_on_aborted_cb cb) {

}

void http_incoming_message_on_close(http_incoming_message_t *message, http_incoming_message_on_close_cb cb) {

}

void http_incoming_message_destroy(http_incoming_message_t *message, int error) {

}

void http_incoming_message_set_timeout(http_incoming_message_t *message, int msecs, http_incoming_message_on_timeout_cb cb) {

}

http_server_t* http_create_server(uv_node_t* h, http_server_on_request_cb cb) {

}

http_client_request_t* http_get(uv_node_t* h,char *url, char **options, char **headers, http_agent_t *agent, http_client_request_response_cb cb) {
    http_client_request_t* req = http_request(h, url, options, headers, agent, cb);
    string_assign_cstr(req->method, "GET");
    http_client_request_end(req, NULL, 0, NULL);
    return req;
}

http_client_request_t* http_request(uv_node_t* h, char *url, char **options, char **headers, http_agent_t *agent, http_client_request_response_cb cb) {
    SAFE_MALLOC(http_client_request_t, req);
    req->h = h;
    req->protocol = create_string();
    string_init_cstr(req->protocol, "http");
    req->host = create_string();
    string_init_cstr(req->host, "localhost");
    req->port = 80;
    req->method = create_string();
    string_init_cstr(req->method, "GET");
    req->path = create_string();
    string_init_cstr(req->path, "/");
    req->headers = create_list(void*);
    list_init(req->headers);
    req->agent = (http_agent_t *)h->http_global_agent;
    req->timeout = 0;
    req->setHost = true;
    req->max_headers_count = 2000;
    req->nodelay = false;

    //解析url
    if(url) {
        http_url_t *u = http_url_parse(url, false);
        if(u->protocol){
            string_assign(req->protocol, u->protocol);
        }
        if(u->auth){
            req->auth = create_string();
            string_init_copy(req->auth, u->auth);
        }
        if(u->host){
            string_assign(req->host, u->host);
        }
        if(u->path){
            string_assign(req->path, u->path);
        }
        http_url_destory(u);
    }

    //使用options
    if(options) {
        int i = 0;
        char *key = options[i], *value = options[i+1];
        for (; key&&value; key = options[i],value = options[i+1]) {
            if(!strcasecmp(key, "protocol")) {
                string_assign_cstr(req->protocol, value);
            } else if(!strcasecmp(key, "host")) {
                string_assign_cstr(req->host, value);
            } else if(!strcasecmp(key, "hostname")) {
                req->hostname = create_string();
                string_init_cstr(req->hostname, value);
            } else if(!strcasecmp(key, "family")) {
                req->family = atoi(value);
            } else if(!strcasecmp(key, "port")) {
                req->port = atoi(value);
            } else if(!strcasecmp(key, "localAddress")) {
                req->localAddress = create_string();
                string_init_cstr(req->localAddress, value);
            } else if(!strcasecmp(key, "method")) {
                string_assign_cstr(req->method, value);
            } else if(!strcasecmp(key, "path")) {
                string_assign_cstr(req->path, value);
            } else if(!strcasecmp(key, "auth")) {
                if(req->auth)
                    string_assign_cstr(req->auth, value);
                else {
                    req->auth = create_string();
                    string_init_cstr(req->auth, value);
                }
            } else if(!strcasecmp(key, "timeout")) {
                req->timeout = atoi(value);
            } else if(!strcasecmp(key, "setHost")) {
                if(!strcasecmp(value, "false"))
                    req->setHost = false;
            }
            i+=2;
        }
    }

    //headers
    if(!headers) {
        int i = 0;
        char *key = options[i], *value = options[i+1];
        string_t *first = create_string(), *second = create_string();
        string_init(first);
        string_init(second);
        for (; key&&value; key = options[i],value = options[i+1]) {
            pair_t *tmp = create_pair(void*, void*);
            string_assign_cstr(first, key);
            string_assign_cstr(second, value);
            pair_init_elem(tmp, first, second);
            i+=2;
        }
    }

    //agents
    if(!agent) {
        if((int)agent == -1) {
            http_agent_options_t agent_opts = {
                false/*keepAlive*/, 1000/*keepAliveMsecs*/, 0/*maxSockets*/, 256/*maxFreeSockets*/, 0/*timeout*/
            };
            req->agent = http_create_agent(h, &agent_opts);
        } else {
            req->agent = agent;
        }
    }

    //使用socket

    return req;
}