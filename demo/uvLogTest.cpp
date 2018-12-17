// uvHttpClient.cpp : 定义控制台应用程序的入口点。
//

#include <stdio.h>
#include <tchar.h>
#include "utilc_api.h"
#include "uvLog.h"

uv_log_handle_t *logger = NULL;
#define MODULE_1_LOGDEBUG(msg, ...) UV_LOG_DEBUG(logger, "moudle1", msg, ##__VA_ARGS__)
#define MODULE_1_LOGINFO(msg, ...)  UV_LOG_INFO (logger, "moudle1", msg, ##__VA_ARGS__)
#define MODULE_1_LOGERROR(msg, ...) UV_LOG_ERROR(logger, "moudle1", msg, ##__VA_ARGS__)

#define MODULE_2_LOGDEBUG(msg, ...) UV_LOG_DEBUG(logger, "moudle2", msg, ##__VA_ARGS__)
#define MODULE_2_LOGINFO(msg, ...)  UV_LOG_INFO (logger, "moudle2", msg, ##__VA_ARGS__)
#define MODULE_2_LOGERROR(msg, ...) UV_LOG_ERROR(logger, "moudle2", msg, ##__VA_ARGS__)

int _tmain(int argc, _TCHAR* argv[])
{
    int ret = uv_log_init(&logger);

    for(;;){
    MODULE_1_LOGDEBUG("this is a test %d", 1);
    MODULE_2_LOGDEBUG("this is a test %d", 2);
    MODULE_1_LOGINFO("info 1");
    MODULE_2_LOGINFO("info2");
    MODULE_1_LOGERROR("error:%d", 1);
    MODULE_2_LOGERROR("error2");
    //break;
    sleep(1000);
    }

    //uv_log_close(logger);
	sleep(INFINITE);
	return 0;
}

