// uvHttpClient.cpp : 定义控制台应用程序的入口点。
//

#include "util.h"
#include "utilc_api.h"
#include "uv.h"
#include "uvnetplus.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <thread>
using namespace std;
using namespace uvNetPlus;

#ifdef _DEBUG    //在Release模式下，不会链接Visual Leak Detector
//#include "vld.h"
#endif

#ifndef INFINITE
#define INFINITE 0XFFFFFFFF
#endif

CNet* net = NULL;
Http::CHttpClient *http = NULL;

struct clientData {
    int tid;
    int finishNum;
    bool err;
    int ref;
};

static void OnHttpError(Http::CHttpRequest *request, string error) {
    clientData* data = (clientData*)request->usrData;
    Log::error("http error[%d] %x: %s",data->tid, data, error.c_str());
    delete data;
}
static void OnHttpResponse(Http::CHttpRequest *request, Http::CHttpMsg* response) {
    clientData* data = (clientData*)request->usrData;
    Log::debug("http response %d %x", data->tid, data);
    Log::debug("%d %s", response->statusCode, response->statusMessage.c_str());
    Log::debug(response->rawHeaders.c_str());
    Log::debug(response->content.c_str());
    if(response->complete && !request->autodel) {
        request->Delete();
    }
    delete data;
}
static void OnSendHttpRequest(Http::CHttpRequest *req, void* usr, std::string error) {
    clientData* data = (clientData*)usr;
    if(error.empty()) {
        Log::debug("new http request %x", req);
        req->usrData    = usr;
        req->OnError    = OnHttpError;
        req->OnResponse = OnHttpResponse;
        req->host       = "www.baidu.com";
        req->protocol   = PROTOCOL::HTTPS;
        req->method     = Http::METHOD::GET;
        req->path       = "/s?wd=http+content+len";
        req->version    = Http::VERSION::HTTP1_1;
        req->End();

    } else {
        Log::error("http error[%d] %x: %s",data->tid, data, error.c_str());
    }
}

void testHttpRequest()
{
    net = CNet::Create();
    http = new Http::CHttpClient(net, 10, 5, 2, 0);
    http->UseTls();
    for (int i=0; i<100; i++) {
        clientData* data = new clientData();
        data->tid = i;
        data->err = false;
        data->ref = 2;
        http->Request("www.baidu.com", 443, data, OnSendHttpRequest);
        if(i%100==99)
            sleep(1);
    }
}

int main()
{
    Log::open(Log::Print::both, Log::Level::debug, "./log/httpsclient.txt");
    testHttpRequest();

	Sleep(INFINITE);
	return 0;
}

