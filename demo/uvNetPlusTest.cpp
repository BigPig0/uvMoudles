// uvHttpClient.cpp : 定义控制台应用程序的入口点。
//

#include "util.h"
#include "utilc.h"
#include "easylog.h"
#include "uvnetplus.h"
#include "uv.h"
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

const int svrport = 8080;
CNet* net = NULL;
Http::CHttpServer *svr = NULL;
Http::CHttpClient *http = NULL;
uv_mutex_t _mutex;

//////////////////////////////////////////////////////////////////////////
////////////       TCP客户端         /////////////////////////////////////

struct clientData {
    int tid;
    int finishNum;
    bool err;
    int ref;
};

void OnClientReady(CTcpSocket* skt){
    clientData* data = (clientData*)skt->userData;
    Log::debug("client %d ready", data->tid);
}

void OnClientRecv(CTcpSocket* skt, char *data, int len){
    clientData* c = (clientData*)skt->userData;
    c->finishNum++;
    Log::debug("client%d recv[%d]: %s", c->tid, c->finishNum, data);
#if 1
    skt->Delete();
#else
    if(c->finishNum < c->tid) {
        char buff[1024] = {0};
        sprintf(buff, "client%d %d",c->tid, c->finishNum+1);
        skt->Send(buff, strlen(buff));
        skt->Send(" AAA", 4);
    } else {
        skt->Delete();
    }
#endif
}

void OnClientDrain(CTcpSocket* skt){
    clientData* data = (clientData*)skt->userData;
    Log::debug("client%d drain", data->tid);
}

void OnClientClose(CTcpSocket* skt){
    clientData* data = (clientData*)skt->userData;
    Log::debug("client%d close", data->tid);
    delete data;
}

void OnClientEnd(CTcpSocket* skt){
    clientData* data = (clientData*)skt->userData;
    Log::debug("client%d end", data->tid);
}

void OnClientError(CTcpSocket* skt, string err){
    clientData* data = (clientData*)skt->userData;
    Log::error("client%d error: %s ", data->tid, err.c_str());
}

void testClient()
{
    CNet* net = CNet::Create();
    for (int i=0; i<100; i++) {
        clientData* data = new clientData;
        data->tid = i+1;
        data->finishNum = 0;
        CTcpSocket* client = CTcpSocket::Create(net, (void*)data);
        client->OnRecv = OnClientRecv;
        client->OnDrain = OnClientDrain;
        client->OnCLose = OnClientClose;
        client->OnEnd = OnClientEnd;
        client->OnError = OnClientError;

        client->OnConnect = [](CTcpSocket* skt, string err){
            clientData* data = (clientData*)skt->userData;
            if(err.empty()){
                char buff[1024] = {0};
                sprintf(buff, "client%d %d",data->tid, data->finishNum+1);
                skt->Send(buff, strlen(buff));
                skt->Send(" 111", 4);
            } else {
                Log::error("client%d connect err: %s", data->tid, err.c_str());
                skt->Delete();
            }
        };
        client->Connect("127.0.0.1", svrport );
    }
}

//////////////////////////////////////////////////////////////////////////
/////////////     tcp客户端连接池      ///////////////////////////////////

static void testTcpPool(){
    std::thread t([&](){
        CNet* net = CNet::Create();
        CTcpConnPool* pool = CTcpConnPool::Create(net);
        pool->maxConns = 5;
        pool->maxIdle  = 5;
        pool->timeOut  = 2;
        pool->maxRequest = 10;
        for (int i=0; i<100; i++) {
            Log::debug("new request %d", i);
            clientData* data = new clientData;
            data->tid = i+1;
            data->finishNum = 0;
            pool->Request("127.0.0.1", svrport, "", data, true, true, [](CTcpRequest* req, CTcpSocket* skt, string err){
                clientData* data = (clientData*)req->usr;
                if(!err.empty()){
                    Log::error("get pool socket fail %d %s",data->tid, err.c_str());
                    delete data;
                    return;
                }

                skt->OnRecv    = OnClientRecv;
                skt->OnDrain   = OnClientDrain;
                skt->OnCLose   = OnClientClose;
                skt->OnEnd     = OnClientEnd;
                skt->OnError   = OnClientError;
                skt->autoRecv  = true;
                skt->copy      = true;
                skt->userData  = data;

                //clientData* ddd = (clientData*)skt->userData;
                //if(ddd){
                //    data->finishNum = ddd->finishNum;
                //    delete ddd;
                //}
                skt->Send("123456789", 9);
                skt->Send("987654321", 9);
            });
        }
    });
    t.detach();
}

//////////////////////////////////////////////////////////////////////////

struct sclientData {
    int id;
};

void testServer() {
    CNet* net = CNet::Create();
    CTcpServer* svr = CTcpServer::Create(net, [](CTcpServer* svr, std::string err, CTcpSocket* client) {
        if(!err.empty()){
            Log::error("tcp server error: %s ", err.c_str());
            return;
        }
        Log::debug("tcp server recv new connection");
        static int sid = 1;
        sclientData *c = new sclientData();
        c->id = sid++;
        client->userData = c;
        client->OnRecv = [](CTcpSocket* skt, char *data, int len){
            sclientData *c = (sclientData*)skt->userData;
            Log::debug("server client%d recv: %s ", c->id, data);
            char buff[1024] = {0};
            sprintf(buff, "server client%d answer",c->id);
            skt->Send(buff, strlen(buff));
            skt->Send("123456", 6);
        };
        client->OnDrain = [](CTcpSocket* skt){
            sclientData *c = (sclientData*)skt->userData;
            Log::debug("server client%d drain", c->id);
        };
        client->OnCLose = [](CTcpSocket* skt){
            sclientData *c = (sclientData*)skt->userData;
            Log::debug("server client%d close", c->id);
            delete c;
        };
        client->OnEnd = [](CTcpSocket* skt){
            sclientData *c = (sclientData*)skt->userData;
            Log::debug("server client%d end", c->id);
        };
        client->OnError = [](CTcpSocket* skt, string err){
            sclientData *c = (sclientData*)skt->userData;
            Log::error("server client%d error:%s ",c->id, err.c_str());
        };
    });
    svr->OnClose = [](CTcpServer* svr, std::string err) {
        if(err.empty()) {
            Log::debug("tcp server closed");
        } else {
            Log::error("tcp server closed with error: %s", err.c_str());
        }
    };
    svr->OnError = [](CTcpServer* svr, std::string err) {
        Log::error("tcp server error: %s", err.c_str());
    };
    svr->OnListen = [](CTcpServer* svr, std::string err) {
        if(err.empty()) {
            Log::debug("tcp server start success");
            //testClient();
            testTcpPool();
        } else {
            Log::error("tcp server start failed: %s ", err.c_str());
        }
    };
    svr->Listen("0.0.0.0", svrport);
}

//////////////////////////////////////////////////////////////////////////

void testHttpRequest()
{
    net = CNet::Create();
    http = new Http::CHttpClient(net, 10, 5, 2, 0);
    for (int i=0; i<10000; i++) {
        clientData* data = new clientData();
        data->tid = i;
        data->err = false;
        data->ref = 2;
        http->Request("www.baidu.com", 80, data, [](Http::CHttpRequest *req, void* usr, std::string error) {
            clientData* data = (clientData*)usr;
            if(error.empty()) {
                Log::debug("new http request %x", req);
                req->usrData    = usr;
                req->OnError    = [](Http::CHttpRequest *request, string error) {
                    clientData* data = (clientData*)request->usrData;
                    Log::error("http error[%d] %x: %s",data->tid, data, error.c_str());
                    //uv_mutex_lock(&_mutex);
                    //data->err = true;
                    //if(request->Finished()) { //主线程的http请求调用过了end才给删除
                    //    request->Delete();
                    //    data->ref--;
                    //    if(!data->ref)
                    //        delete data;
                    //}
                    //uv_mutex_unlock(&_mutex);
                    delete data;
                };
                req->OnResponse = [](Http::CHttpRequest *request, Http::CHttpMsg* response) {
                    clientData* data = (clientData*)request->usrData;
                    Log::debug("http response %d %x", data->tid, data);
                    Log::debug("%d %s", response->statusCode, response->statusMessage.c_str());
                    Log::debug(response->rawHeaders.c_str());
                    Log::debug(response->content.c_str());
                    if(response->complete && !request->autodel) {
                        request->Delete();
                    }
                    delete data;
                };

                if(1){
                    req->host       = "www.baidu.com";
                    req->protocol   = PROTOCOL::HTTP;
                    req->method     = Http::METHOD::GET;
                    req->path     = "/s?wd=http+content+len";
                    req->version    = Http::VERSION::HTTP1_1;
                    req->End();
                } else {
                    req->host     = "127.0.0.1";
                    req->method   = Http::METHOD::POST;
                    req->path       = "/imageServer/image?name=111111.jpg&type=1";
                    req->version    = Http::VERSION::HTTP1_1;
                    req->SetHeader("myheader","????????\0");
                    string content = "123456789\0";
                    req->End(content.c_str(), content.length());
                }
            } else {
                Log::error("http error[%d] %x: %s",data->tid, data, error.c_str());
            }
        });
        //http->Request("127.0.0.1", 80, data, OnSendHttpRequest);
        if(i%100==99)
            sleep(1);
    }
}

void testHttpServer()
{
    net = CNet::Create();
    svr = Http::CHttpServer::Create(net);
    svr->OnRequest = [](Http::CHttpServer *server, Http::CHttpMsg *request, Http::CHttpResponse *response) {
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
    };
    svr->Listen("0.0.0.0", svrport);
}

//////////////////////////////////////////////////////////////////////////
#ifdef WINDOWS_IMPL
BOOL CtrlCHandler(DWORD type)
{
    switch(type)
    {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
        delete http;
        Sleep(5000);
        //delete net;
        Sleep(2000);
        if(type == CTRL_C_EVENT)
            exit(0);
        return TRUE;
    default:
        return FALSE;
    }
    return FALSE;
}
#else
void CtrlCHandler(int sig) {
    delete http;
}
#endif

int main()
{
    /** 设置控制台消息回调 */
    //SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlCHandler, TRUE); 
    uv_mutex_init(&_mutex);
    Log::open(Log::Print::both, uvLogPlus::Level::Debug, "./log/log.txt");
    testServer();
    //testTcpPool();
    //testHttpServer();
    //testHttpRequest();

	sleep(INFINITE);
	return 0;
}

