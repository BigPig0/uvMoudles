#ifndef _TYPEDEF_
#define _TYPEDEF_

#ifdef __cplusplus
extern "C" {
#endif

#include "cstl.h"
#include "uv.h"
#include "memfile.h"

#define SOCKET_RECV_BUFF_LEN 1024*1024

/** http环境结构 */
typedef struct _http_ {
    config_t    conf;
    map_t*      agents;
	uv_loop_t*  uv;
    uv_timer_t  timeout_timer;
	bool_t      inner_uv;
	bool_t      is_run;
	uv_mutex_t  uv_mutex_h;
}http_t;

typedef struct _response_p_ response_p_t;
/** 请求数据结构 */
typedef struct _request_p_ {
	HTTP_METHOD    method;
	const char*    url;
	const char*    host;
    int            keep_alive;
	int            chunked;
	int            content_length;
	void*          user_data;
	response_p_t*  res;

    string_t*      str_host;	//根据url或host生成的host地址
	string_t*      str_addr;    //host域名部分
	string_t*      str_port;    //host端口部分
	string_t*      str_path;    //uri的path部分
    struct sockaddr* addr;      //tcp连接地址
    string_t*      str_header;  //根据用户填写的内容生成的http头
    http_t*        handle;
    map_t*         headers;     //用户填写的http头 map<string,string>
	list_t*        body;        //用户填写的http内容体 list<membuff>
	uv_mutex_t     uv_mutex_h;

	request_cb     req_cb;
	response_data  res_data; 
	response_cb    res_cb;
}request_p_t;

typedef enum _response_step_
{
    response_step_notbegin = 0,
    response_step_protocol,     //找到http/1.x
    response_step_stause_begin, //
    response_step_status_ok,    //解析了状态码
    response_step_status_desc,  //解析了状态说明文字
    response_step_header_key,   //解析了应答头字段
    response_step_header_value, //解析了应答头值
    response_step_header_end    //http头解析完毕
}res_step_t;

/** 应答数据结构 */
typedef struct _response_p_ {
	int           status;         //应答状态码
	int           keep_alive;     //0表示Connection为close，非0表示keep-alive
	int           chunked;        //0表示不使用chuncked，非0表示Transfer-Encoding: "chunked"
	int           content_length; //内容的长度；一个chunk后内容的长度
	request_p_t*  req;

    http_t*       handle;
    map_t*        headers;        //解析后的应答头内容

	res_step_t    parsed_headers; //应答解析状态
    memfile_t*    header_buff;    //临时存放接收的数据
	int           recived_length; //接收的内容长度；接收的该chunk的长度
	//string_t*     chunk_left;     //一次接收的缓冲区末尾chunk长度没有结束时的内容
	uv_mutex_t    uv_mutex_h;   
}response_p_t;

/** 客户端数据结构 */
typedef struct _agent_
{
    http_t*     handle;
    list_t*     req_list;
    set_t*      sockets;		//工作中的连接
    set_t*      free_sockets;   //空闲可用连接
	list_t*     requests;       //任务链表
    bool_t      keep_alive;
	uv_mutex_t  uv_mutex_h;
}agent_t;

/** tcp连接状态 */
typedef enum _socket_status_
{
    socket_uninit = 0,
    socket_init,
    socket_connected,
    socket_send,
    socket_recv,
    socket_closed
}socket_status_t;

/** tcp连接数据结构 */
typedef struct _socket_
{
    agent_t*        agent;
    request_p_t*    req;
    socket_status_t status;
	int             isbusy; //1位于sockets队列，0位于free_sockets队列

    uv_tcp_t        uv_tcp_h;
    uv_connect_t    uv_connect_h;  
	uv_write_t      uv_write_h;
	uv_mutex_t      uv_mutex_h;

	char            buff[SOCKET_RECV_BUFF_LEN];
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
	uv_http_err_dns_parse,
    uv_http_err_connect,
    uv_http_err_remote_disconnect,
    uv_http_err_local_disconnect
}err_code_t;


#ifdef WIN32
#define fieldcmp _stricmp
#else
#include <strings.h>
#define fieldcmp strcasecmp
#endif

extern void agents_init(http_t* h);
extern void agents_destory(http_t* h);

extern int agent_request(request_p_t* req);
extern void agent_request_finish(bool_t ok, socket_t* socket);
extern void destory_request(request_p_t* req);
extern void generic_request_header(request_p_t* req);
extern int parse_dns(request_p_t* req);

extern socket_t* create_socket(agent_t* agent);
//extern void agent_free_socket(socket_t* socket);
extern void destory_socket(socket_t* socket);
extern void socket_run(socket_t* socket);

extern response_p_t* create_response(request_p_t* req);
extern void destory_response(response_p_t* res);
extern bool response_recive(response_p_t* res, char* data, int len);
extern void response_error(response_p_t* res, int code);
extern void string_map_compare(const void* cpv_first, const void* cpv_second, void* pv_output);

#ifdef __cplusplus
}
#endif
#endif