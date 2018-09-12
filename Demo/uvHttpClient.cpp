// uvHttpClient.cpp : 定义控制台应用程序的入口点。
//

#include <stdio.h>
#include <tchar.h>
#include "uvHttpPlus.h"

void request_cb_h(CRequest* req, int code)
{
}

void response_data_h(CRequest* req, char* data, int len)
{
}

void response_cb_h(CRequest* req, int code)
{

}

int _tmain(int argc, _TCHAR* argv[])
{
	config_t cof;
	cof.keep_alive_secs = 0;
	cof.max_sockets = 10;
	cof.max_free_sockets = 10;
	CHttpPlus http(cof, nullptr);
	for (int i = 0; i < 100; ++i)
	{
		CRequest* req = http.CreatRequest(request_cb_h, response_data_h, response_cb_h);
		req->SetURL("http://www.baidu.com");
		req->SetMethod(HTTP_METHOD::METHOD_GET);
		req->Request();
	}
	return 0;
}

