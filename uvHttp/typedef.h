#ifndef _TYPEDEF_
#define _TYPEDEF_

#ifdef __cplusplus
extern "C" {
#endif

#include "cstl.h"
#include "uv.h"
#include "stdbool.h"

/** http环境结构 */
typedef struct _http_ {
    config_t    conf;
    map_t*      agents;
	uv_loop_t*  uv;
    uv_timer_t  timeout_timer;
	bool        inner_uv;
	bool        is_run;
}http_t;

typedef struct _response_p_ response_p_t;
/** 请求数据结构 */
typedef struct _request_p_ {
	const char*    url;
    const char*    method;
	const char*    host;
    int            keep_alive;
	int            content_length;
	void*          user_data;
	response_p_t*  res;

    string_t*      str_host;	//host地址
	string_t*      str_addr;    //host域名部分
	string_t*      str_port;    //host端口部分
	string_t*      str_path;    //uri的path部分
    http_t*        handle;
    map_t*         headers;
	list_t*        body;

	request_cb     req_cb;
	response_data  res_data; 
	response_cb    res_cb;
}request_p_t;

/** 应答数据结构 */
typedef struct _response_p_ {
	const char*   version;
	int           status;
	int           keep_alive;     //0表示Connection为close，非0表示keep-alive
	int           chunked;        //POST使用 0表示不使用chuncked，非0表示Transfer-Encoding: "chunked"
	int           content_length; //POST时需要标注内容的长度
	request_p_t*  req;

    http_t*       handle;
    map_t*        headers;
}response_p_t;

/** 客户端数据结构 */
typedef struct _agent_
{
    http_t*     handle;
    list_t*     req_list;
    set_t*      sockets;		//工作中的连接
    set_t*      free_sockets;   //空闲可用连接
	list_t*     requests;       //任务链表
    bool        keep_alive;
}agent_t;

/** tcp连接数据结构 */
typedef struct _socket_
{

}socket_t;

/** 内存数据结构 */
typedef struct _membuff_
{
	unsigned char* data;
	unsigned int len;
}membuff_t;

/** 错误码 */
typedef enum _err_code_
{
    uv_http_ok = 0,
	uv_http_err_protocol,
	uv_http_err_dns_parse
}err_code_t;

#ifdef __cplusplus
}
#endif
#endif