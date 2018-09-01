#ifndef _TYPEDEF_
#define _TYPEDEF_

#include "cstl.h"
#include "uv.h"
#include "uvHttp.h"

typedef struct _http_ {
    config_t    conf;
    map_t*      agents;
    uv_timer_t  timeout_timer;
}http_t;

typedef struct _response_ response_t;
typedef struct _request_ {
    http_t*     handle;
    response_t* res;
    void*       user_data;
    const char* method;
    const char* path;
    const char* version;
    const char* host;
    const char* port;
    bool        keep_alive;
    int         content_length;
    map_t*      headers;
}request_t;

typedef struct _response_ {
    http_t*     handle;
    request_t*  req;
    map_t*      headers;
}response_t;


typedef struct _agent_
{
    http_t*     handle;
    list_t*     req_list;
    set_t*      sockets;
    set_t*      free_sockets;
    bool        keep_alive;
}agent_t;

typedef enum _err_code_
{
    uv_http_ok = 0
}err_code_t;

#endif