// uvHttpClient.cpp : 定义控制台应用程序的入口点。
//

#include <stdio.h>
#include <tchar.h>
#include "uvHttpPlus.h"
#include <windows.h>

void request_cb_h(CRequest* req, int code)
{
    printf("request_cb_h: %d", code);
}

void response_data_h(CRequest* req, char* data, int len)
{
    printf("response_data_h: %d", len);
}

void response_cb_h(CRequest* req, int code)
{
    printf("response_cb_h: %d", code);
}

int _tmain(int argc, _TCHAR* argv[])
{
	config_t cof;
	cof.keep_alive_secs = 0;
	cof.max_sockets = 10;
	cof.max_free_sockets = 10;
	CHttpPlus* http = new CHttpPlus(cof, nullptr);
	for (int i = 0; i < 1; ++i)
	{
		CRequest* req = http->CreatRequest(request_cb_h, response_data_h, response_cb_h);
		req->SetURL("http://www.baidu.com");
		req->SetMethod(HTTP_METHOD::METHOD_GET);
		req->SetContentLength(0);
		req->Request();
	}
	Sleep(INFINITE);
	return 0;
}

