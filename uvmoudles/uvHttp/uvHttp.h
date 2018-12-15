#ifndef _UV_HTTP_
#define _UV_HTTP_ 

#ifdef __cplusplus
extern "C" {
#endif

#include "public_def.h"


/**
 * 初始化http环境
 * @param cof 该http环境的配置
 * @param uv 外部输入uv_loop_t循环句柄，当输入null时，由http环境内部创建一个自己的uvloop
 * @return 返回一个http环境句柄
 */
extern http_t* uvHttp(config_t cof, void* uv);

/**
 * 关闭一个http环境句柄
 * @param h 输入句柄
 */
extern void uvHttpClose(http_t* h);

/**
 * 建立一个http客户端请求
 * @param h http环境句柄
 * @param req_cb 请求发送后的回调函数
 * @param res_data 收到应答的body的回调
 * @param res_cb 应答接收完成后的回调
 * @return http请求句柄。该句柄由uvhttp调度完成后自动释放
 */
extern request_t* creat_request(http_t* h, request_cb req_cb, response_data res_data, response_cb res_cb);

/**
 * 添加http头内容
 * @param req http环境句柄
 * @param key 头域名称
 * @param value 头域值
 */
extern void add_req_header(request_t* req, const char* key, const char* value);

/**
 * 向请求中添加http body内容，仅POST时才需要
 * @param req http请求句柄
 * @param data body内容。指针的内容有调用者维护释放
 * @param len body的长度
 * @return 错误码
 * @note  该方法在request之前使用,可以调用多次；也可在request的回调方法中使用request_write继续写入
 */
extern int add_req_body(request_t* req, const char* data, int len);

/**
 * 获取应答中http协议版本
 * @param res http应答句柄
 */
extern char* get_res_version(response_t* res);

/**
 * 获取应答中的状态描述文字
 * @param res http应答句柄
 */
extern char* get_res_status_des(response_t* res);

/**
 * 获取应答中的头域数量
 * @param res http应答句柄
 */
extern int get_res_header_count(response_t* res);

/**
 * 获取应答中头域指定位置的头域名称
 * @param i 指定位置
 * @return 应答的头域名称
 */
extern char* get_res_header_name(response_t* res, int i);

/**
 * 获取应答中头域指定位置的头域的值
 * @param i 指定位置
 * @return 应答的头域值
 */
extern char* get_res_header_value(response_t* res, int i);

/**
 * 根据头域名称获取对应的值
 * @param key 应答的头域的名称
 * @return 应答的头域值
 */
extern char* get_res_header(response_t* res, const char* key);

/**
 * 发送http请求
 * @param req http请求的句柄
 * @return 错误码
 */
extern int request(request_t* req);

/**
 * 继续发送http请求的body内容
 * @param req http请求的句柄
 * @param data body数据
 * @param len body长度
 * @note 这个方法在request_cb回调方法里面调用
 */
extern int request_write(request_t* req, char* data, int len);


#ifdef __cplusplus
}
#endif

#endif