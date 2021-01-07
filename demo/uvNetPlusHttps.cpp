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

//开启这个宏的时候，使用项目路径下的证书文件；不开启的时候使用tls库里的默认测试证书内容
//证书里的域名是localhost
#define USE_CRT_FILE
//定义客户端请求次数
#define CLIENT_NUM 100
//定义服务端监听端口
const int svrport = 6789;

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
        req->host       = "localhost";
        req->protocol   = PROTOCOL::HTTP;
        req->method     = Http::METHOD::POST;
        req->path       = "/test";
        req->version    = Http::VERSION::HTTP1_1;

        req->SetHeader("myheader","????????\0");
        string content = "123456789\0";
        req->End(content.c_str(), content.length());

    } else {
        Log::error("http error[%d] %x: %s",data->tid, data, error.c_str());
    }
}

void testHttpRequest()
{
    CNet *net = CNet::Create();
    Http::CHttpClient *http = new Http::CHttpClient(net, 10, 5, 2, 0);
#ifdef USE_CRT_FILE
    const char *caArray[] = {"./ssl/ca.crt", NULL};
    http->SetTlsCaFile(caArray);
    http->UseTls(true);
#endif
    for (int i=0; i<CLIENT_NUM; i++) {
        clientData* data = new clientData();
        data->tid = i;
        data->err = false;
        data->ref = 2;
        http->Request("localhost", svrport, data, OnSendHttpRequest);
        if(i%100==99)
            sleep(1);
    }
}

static void OnSvrListen(Http::CHttpServer* svr, std::string err) {
    testHttpRequest();
}

static void OnRcvHttpRequest(Http::CHttpServer *server, Http::CHttpMsg *request, Http::CHttpResponse *response) {
    Log::debug("%d %s %d", request->method, request->path.c_str(), request->version);
    for(auto &h:request->headers) {
        Log::debug("%s: %s", h.first.c_str(), h.second.c_str());
    }
    Log::debug("Content len: %d", request->contentLen);
    Log::debug("Chunked: %d", request->chunked);
    Log::debug("Keep alive: %d", request->keepAlive);
    Log::debug("Complete: %d", request->complete);
    Log::debug(request->rawHeaders.c_str());

    response->statusCode = 200;
    response->SetHeader("myheader", "1111111111111");
    response->End("123456789",9);
}

void testHttpServer()
{
    CNet *net = CNet::Create();
    Http::CHttpServer *svr = Http::CHttpServer::Create(net);
#ifdef USE_CRT_FILE
    svr->SetCAFile("./ssl/ca.crt");
    svr->SetCrtFile("./ssl/server.crt");
    svr->SetKeyFile("./ssl/server.key");
#endif
    svr->UseTls();
    svr->OnListen = OnSvrListen;
    svr->OnRequest = OnRcvHttpRequest;
    svr->Listen("0.0.0.0", svrport);
}

int main()
{
    Log::open(Log::Print::both, Log::Level::debug, "./log/uvNetPlusHttps/log.txt");
    testHttpServer();

	sleep(INFINITE);
	return 0;
}

