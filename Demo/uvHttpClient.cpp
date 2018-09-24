// uvHttpClient.cpp : 定义控制台应用程序的入口点。
//

#include <stdio.h>
#include <tchar.h>
#include "uvHttpPlus.h"
#include <windows.h>

void request_cb_h(CRequest* req, int code)
{
    printf("request_cb_h: %d %s\r\n", code, uvhttp_err_msg(code));
}

void response_data_h(CRequest* req, char* data, int len)
{
    printf("response_data_h: %d\r\n", len);
}

void response_cb_h(CRequest* req, int code)
{
    printf("response_cb_h: %d %s\r\n", code, uvhttp_err_msg(code));
}

string strUrl[] = {
    //"http://www.baidu.com",
    //"http://180.97.33.107/",
    //"http://180.97.33.108/",
    //"http://www.baidu.com/s?wd=qq",
    //"http://180.97.33.107/s?wd=qq",
    //"http://180.97.33.108/s?wd=qq",
    //"http://www.baidu.com/s?wd=ww",
    //"http://180.97.33.107/s?wd=ww",
    //"http://180.97.33.108/s?wd=ww",
    "http://www.sleuthkit.org/sleuthkit/docs/api-docs/4.6/group__imglib.html",
};

int _tmain(int argc, _TCHAR* argv[])
{
	config_t cof;
	cof.keep_alive_secs = 0;
	cof.max_sockets = 10;
	cof.max_free_sockets = 10;
	CHttpPlus* http = new CHttpPlus(cof, nullptr);
	for (int i = 0; i < 10; ++i)
	{
		CRequest* req = http->CreatRequest(request_cb_h, response_data_h, response_cb_h);
        string url = strUrl[i % (sizeof(strUrl) / sizeof(string))];
        char num[5] = { 0 };
        sprintf_s(num, 4, "%d", i);
		req->SetURL(url+num);
		req->SetMethod(HTTP_METHOD::METHOD_GET);
		req->SetContentLength(0);
		req->Request();
        Sleep(10000);
	}
	Sleep(INFINITE);
	return 0;
}

