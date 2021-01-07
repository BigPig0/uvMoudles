#include "util.h"
#include "utilc.h"
#include "uvnetplus.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <thread>
#include <windows.h>

using namespace std;
using namespace uvNetPlus;

//定义客户端请求次数
#define CLIENT_NUM 100

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
    http->UseTls(); // 没有设置CA，verify=false不检查服务器的证书是否有效
    for (int i=0; i<CLIENT_NUM; i++) {
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
    Log::open(Log::Print::both, Log::Level::debug, "./log/uvNetPlusHttpsClient/log.txt");
    testHttpRequest();

	Sleep(INFINITE);
	return 0;
}

