#ifndef _UV_NODE_
#define _UV_NODE_ 

#ifdef __cplusplus
extern "C" {
#endif

#include "public.h"


/**
 * 初始化环境
 * @param uv 外部输入uv_loop_t循环句柄，当输入null时，由内部创建一个自己的uvloop
 * @return 返回一个环境句柄
 */
extern uv_node_t* uv_node_create(void* uv);

/**
 * 关闭一个环境句柄
 * @param h 输入句柄
 * @note 关闭前需要确保该环境下的工作全部先关闭
 */
extern void uv_node_close(uv_node_t* h);

#ifdef __cplusplus
}
#endif

#endif