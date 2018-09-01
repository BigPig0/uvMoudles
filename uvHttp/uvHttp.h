#ifndef _UV_HTTP_
#define _UV_HTTP_ 

#ifdef __cplusplus
extern "C" {
#endif

#include "cstl.h"

typedef struct _config_
{
    int  keep_alive_secs;
    int  max_sockets;
    int  max_free_sockets;
}config_t;

typedef struct _option_
{
    const char* method;
    const char* path;
    const char* version;
    const char* host;
    const char* port;
    bool        keep_alive;
    int         content_length;
    map_t*      headers;
}option_t;

typedef struct _err_info_
{
    int         error;
    const char* message;
}err_info_t;


typedef struct _http_        http_t;
typedef struct _request_     request_t;
typedef struct _response_    response_t;
typedef enum   _err_code_    err_code_t;


/** 公共的工具方法 */
extern string_t* url_encode(string_t* src);
extern string_t* url_decode(string_t* src);

/** 模块加载、卸载 */
extern http_t* uvHttp(config_t cof);
extern void ubHttpClose(http_t* h);

/** 发送请求，回调里面处理写实体数据和设置应答回调 */
typedef void (*request_cb)(err_info_t, request_t*); 
extern request_t* creat_request(http_t* h);
extern int request(request_t* req, request_cb cb);

/** 向请求中写入实体数据，只有POST才会用到 */
typedef void (*req_write_cb)(err_info_t);
extern int request_write(request_t* req, char* data, int len, req_write_cb cb);

/** 设置收到应答的内容 */
typedef void (*response_data)(char*, int);
extern int on_response_data(response_t* res, response_data cb);

/** 设置应答接收完成的处理 */
typedef void (*response_end)();
extern int on_response_end(response_t* res, response_end cb);


#ifdef __cplusplus
}
#endif

#endif