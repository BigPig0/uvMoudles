#ifndef _TYPEDEF_
#define _TYPEDEF_

#ifdef __cplusplus
extern "C" {
#endif

#include "cstl.h"
#include "uv.h"

#define SOCKET_RECV_BUFF_LEN 1024*1024

/** http环境结构 */
typedef struct _http_ {
    config_t    conf;
    map_t*      agents;
	uv_loop_t*  uv;
    uv_timer_t  timeout_timer;
	bool_t      inner_uv;
	bool_t      is_run;
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

	request_cb     req_cb;
	response_data  res_data; 
	response_cb    res_cb;
}request_p_t;

/** 应答数据结构 */
typedef struct _response_p_ {
	int           status;         //应答状态码
	int           keep_alive;     //0表示Connection为close，非0表示keep-alive
	int           chunked;        //0表示不使用chuncked，非0表示Transfer-Encoding: "chunked"
	int           content_length; //内容的长度；一个chunk后内容的长度
	request_p_t*  req;

    http_t*       handle;
    map_t*        headers;

	int           parsed_headers; //0未解析头，1已经解析头
	int           recived_length; //接收的内容长度；接收的该chunk的长度
	string_t*     chunk_left;     //一次接收的缓冲区末尾chunk长度没有结束时的内容
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
	uv_mutex_t      uv_mutex_t;

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
    uv_http_err_connect
}err_code_t;

#ifdef __cplusplus
}
#endif
#endif