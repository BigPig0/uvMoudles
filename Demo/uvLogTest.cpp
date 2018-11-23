// uvHttpClient.cpp : 定义控制台应用程序的入口点。
//

#include <stdio.h>
#include <tchar.h>
#include "util_api.h"
#include "uvLog.h"

int _tmain(int argc, _TCHAR* argv[])
{
    uv_log_handle_t *logger = NULL;
    int ret = uv_log_init(&logger);
    uv_log_close(logger);
	sleep(INFINITE);
	return 0;
}

