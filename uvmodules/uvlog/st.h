#ifndef _UV_LOG_ST_
#define _UV_LOG_ST_

#ifdef __cplusplus
extern "C" {
#endif

#include "cstl.h"
#include "uv.h"
#include "uvLog.h"
#include "ring_buff.h"

typedef enum _appender_type_ {
    consol = 0,
    file,
    rolling_file
}appender_type_t;

typedef enum _consol_target_ {
    SYSTEM_OUT = 0,
    SYSTEM_ERR
}consol_target_t;

typedef enum _filter_match_ {
    ACCEPT = 0,     //接受
    NEUTRAL,        //中立
    DENY            //拒绝
}filter_match_t;

typedef struct _filter_ {
    level_t        level;
    filter_match_t on_match;
    filter_match_t mis_match;
}filter_t;

typedef struct _time_based_triggering_policy_ {
    int             interval;           //指定多久滚动一次，默认是1 hour
    bool_t          modulate;           //用来调整时间：比如现在是早上3am，interval是4，那么第一次滚动是在4am，接着是8am，12am...而不是7am
}time_based_triggering_policy_t;

typedef struct _size_based_triggering_policy_ {
    int             size;               //定义每个日志文件的大小
}size_based_triggering_policy_t;

typedef struct _policies_ {
    time_based_triggering_policy_t  time_policy;
    size_based_triggering_policy_t  size_policy;
}policies_t;

typedef struct _default_rollover_strategy_
{
    int             max;                //指定同一个文件夹下最多有几个日志文件时开始删除最旧的,默认为最多同一文件夹下7个文件
}default_rollover_strategy_t;

#define APPENDER_PUBLIC \
    appender_type_t type;\
    string_t        *name;\
    string_t        *pattern_layout;\
    list_t          *filter;
//type 指定appender的类型
//name 指定Appender的名字
//pattern_layout 输出格式，不设置默认为:%m%n
//filter <filter_t*>的link list

/** appender基础类型 */
typedef struct _appender_ {
    APPENDER_PUBLIC
}appender_t;

/** 控制台输出appender */
typedef struct _consol_appender_ {
    APPENDER_PUBLIC
    consol_target_t target;             //一般只设置默认:SYSTEM_OUT
    uv_tty_t        tty_handle;         //控制台句柄
    bool_t          opening;            //正在打开控制台
    bool_t          opened;             //控制台已经打开
}consol_appender_t;

/** 单文件输出appender */
typedef struct _file_appender_ {
    APPENDER_PUBLIC
    string_t        *file_name;         //指定输出日志的目的文件带全路径的文件名
    bool_t          append;             //是否追加，默认false
    uv_file         file_handle;        //打开的文件句柄
    bool_t          opening;            //正在打开文件
    bool_t          opened;             //文件已经打开
}file_appender_t;

/** 动态文件输出appender */
typedef struct _rolling_file_appender_ {
    APPENDER_PUBLIC
    string_t        *filePattern;       //指定新建日志文件的名称格式.
    policies_t      policies;           //指定滚动日志的策略，就是什么时候进行新建日志文件输出日志.
    default_rollover_strategy_t drs;    //用来指定同一个文件夹下最多有几个日志文件时开始删除最旧的，创建新的(通过max属性)。
    uv_file         file_handle;        //打开的文件句柄
    bool_t          opening;            //正在打开文件
    bool_t          opened;             //文件已经打开
}rolling_file_appender_t;

/** 一个日志实例 */
typedef struct _logger_ {
    level_t         level;
    string_t        *name;              //指定该Logger所适用的类或者类所在的包全路径,继承自Root节点
    list_t          *appender_ref;      //用来指定该日志输出到哪个Appender,如果没有指定，就会默认继承自Root
    bool_t          additivity;         //appender_ref指定了值，是否仍然输出到root
    ring_buff_t     *ring;              //存放日志任务的环形缓冲区
}logger_t;

/** 全局配置 */
typedef struct _configuration_ {
    level_t         status;             //用来指定log4j本身的打印日志的级别
    int             monitorinterval;    //指定log4j自动重新配置的监测间隔时间，单位是s,最小是5s
    hash_map_t      *appenders;
    logger_t        *root;
    hash_map_t      *loggers;           //配置生成之后，只有多线程读取，可以无锁
}configuration_t;

/** 一条日志任务消息 */
typedef struct _task_log_msg {
    uint32_t    tid;
    level_t     level;
    char        *file_name;
    char        *func_name;
    int         line;
    time_t      msg_time;
    string_t    *msg;
}task_log_msg_t;

/** 日志任务队列 */
typedef struct _task_queue {
    volatile int task_id;
    map_t        *queue;  //int:task_log_msg_t*
    uv_rwlock_t  lock;
}task_queue_t;

/**  */
typedef struct _task_fifo_queue_ {
    volatile int last_queue_id; //只能是0或1
    volatile int queue_id;      //只能是0或1
    task_queue_t queue[2];
}task_fifo_queue_t;

/** uvLog实例句柄 */
typedef struct _uv_log_handle_ {
    configuration_t *config;
    uv_loop_t       loop;
    //uv_timer_t      task_timer;
    uv_async_t       async;
    //task_fifo_queue_t *task_queue;
}uv_log_handle_t;

/** 写日志请求 */
typedef struct _task_log_msg_req_ {
    uv_log_handle_t *handle;
    logger_t        *logger;
    appender_t      *appender;
    uv_buf_t         buff;
}task_log_msg_req_t;

extern filter_t*        create_filter();
extern consol_appender_t*        create_appender_consol();
extern file_appender_t*          create_appender_file();
extern rolling_file_appender_t*  create_appender_rolling_file();
extern logger_t*        create_logger();

extern configuration_t* create_config();
extern void destory_config(configuration_t* conf);

extern task_log_msg_t* create_task_log_msg();
extern void destory_task_log_msg(task_log_msg_t *task);

extern task_fifo_queue_t* create_task_fifo_queue();
extern void destory_task_fifo_queue(task_fifo_queue_t *queue);

extern void add_task(task_fifo_queue_t *queue, task_log_msg_t* task);
typedef void (*task_func)(uv_log_handle_t* h, task_log_msg_t* task);
extern void get_task(uv_log_handle_t* h, task_fifo_queue_t *queue, task_func cb);

#ifdef __cplusplus
}
#endif
#endif