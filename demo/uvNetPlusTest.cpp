// uvHttpClient.cpp : 定义控制台应用程序的入口点。
//

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include "uvnetplus.h"
#include "Log.h"
#include <string>
#include <thread>
using namespace std;
using namespace uvNetPlus;

const int svrport = 8080;

//////////////////////////////////////////////////////////////////////////
////////////       TCP客户端         /////////////////////////////////////

struct clientData {
    int tid;
    int finishNum;
};

void OnClientReady(CTcpClient* skt){
    clientData* data = (clientData*)skt->userData;
    Log::debug("client %d ready", data->tid);
}

void OnClientConnect(CTcpClient* skt, string err){
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
}

void OnClientRecv(CTcpClient* skt, char *data, int len){
    clientData* c = (clientData*)skt->userData;
    c->finishNum++;
    Log::debug("client%d recv[%d]: %s", c->tid, c->finishNum, data);
    //skt->Delete();
    if(c->finishNum < c->tid) {
        char buff[1024] = {0};
        sprintf(buff, "client%d %d",c->tid, c->finishNum+1);
        skt->Send(buff, strlen(buff));
        skt->Send(" AAA", 4);
    } else {
        skt->Delete();
    }
}

void OnClientDrain(CTcpClient* skt){
    clientData* data = (clientData*)skt->userData;
    Log::debug("client%d drain", data->tid);
}

void OnClientClose(CTcpClient* skt){
    clientData* data = (clientData*)skt->userData;
    Log::debug("client%d close", data->tid);
    delete data;
}

void OnClientEnd(CTcpClient* skt){
    clientData* data = (clientData*)skt->userData;
    Log::debug("client%d end", data->tid);
}

void OnClientError(CTcpClient* skt, string err){
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
        CTcpClient* client = CTcpClient::Create(net, (void*)data);
        client->OnRecv = OnClientRecv;
        client->OnDrain = OnClientDrain;
        client->OnCLose = OnClientClose;
        client->OnEnd = OnClientEnd;
        client->OnError = OnClientError;
        client->OnConnect = OnClientConnect;
        client->Connect("127.0.0.1", svrport );
    }
}

//////////////////////////////////////////////////////////////////////////
/////////////     tcp客户端连接池      ///////////////////////////////////

static void OnConnectRequest(CTcpRequest* req, std::string error){
    clientData* data = (clientData*)req->usr;
    if(!error.empty())
        Log::error("OnConnectRequest client%d error: %s ", data->tid, error.c_str());
}

static void OnConnectResponse(CTcpRequest* req, std::string error, const char *data, int len){
    clientData* cli = (clientData*)req->usr;
    if(!error.empty())
        Log::error("OnConnectResponse fialed client%d error: %s ", cli->tid, error.c_str());
    else
        Log::debug("OnConnectResponse ok req%d recv%s", cli->tid, data);
    req->Finish();
}

static void testTcpPool(){
    std::thread t([&](){
        CNet* net = CNet::Create();
        CTcpConnPool* client = CTcpConnPool::Create(net, OnConnectRequest, OnConnectResponse);
        client->maxConns = 10;
        client->maxIdle = 10;
        for (int i=0; i<500; i++) {
            Log::debug("new request %d", i);
            clientData* data = new clientData;
            data->tid = i+1;
            data->finishNum = 0;
            CTcpRequest* tcpReq = client->Request("127.0.0.1", svrport, "", data, true, true);
            tcpReq->Request("123456789", 9);
            tcpReq->Request("987654321", 9);
        }
    });
    t.detach();
}

//////////////////////////////////////////////////////////////////////////

struct sclientData {
    int id;
};

void OnSvrCltRecv(CTcpClient* skt, char *data, int len){
    sclientData *c = (sclientData*)skt->userData;
    Log::debug("server client%d recv: %s ", c->id, data);
    char buff[1024] = {0};
    sprintf(buff, "server client%d answer",c->id);
    skt->Send(buff, strlen(buff));
    skt->Send("123456", 6);
}

void OnSvrCltDrain(CTcpClient* skt){
    sclientData *c = (sclientData*)skt->userData;
    Log::debug("server client%d drain", c->id);
}

void OnSvrCltClose(CTcpClient* skt){
    sclientData *c = (sclientData*)skt->userData;
    Log::debug("server client%d close", c->id);
    delete c;
}

void OnSvrCltEnd(CTcpClient* skt){
    sclientData *c = (sclientData*)skt->userData;
    Log::debug("server client%d end", c->id);
}

void OnSvrCltError(CTcpClient* skt, string err){
    sclientData *c = (sclientData*)skt->userData;
    Log::error("server client%d error:%s ",c->id, err.c_str());
}

void OnTcpConnection(CTcpServer* svr, std::string err, CTcpClient* client) {
    if(!err.empty()){
        Log::error("tcp server error: %s ", err.c_str());
        return;
    }
    Log::debug("tcp server recv new connection");
    static int sid = 1;
    sclientData *c = new sclientData();
    c->id = sid++;
    client->userData = c;
    client->OnRecv = OnSvrCltRecv;
    client->OnDrain = OnSvrCltDrain;
    client->OnCLose = OnSvrCltClose;
    client->OnEnd = OnSvrCltEnd;
    client->OnError = OnSvrCltError;
}

void OnTcpSvrClose(CTcpServer* svr, std::string err) {
    if(err.empty()) {
        Log::debug("tcp server closed");
    } else {
        Log::error("tcp server closed with error: %s", err.c_str());
    }
}

void OnTcpError(CTcpServer* svr, std::string err) {
    Log::error("tcp server error: %s", err.c_str());
}

void OnTcpListen(CTcpServer* svr, std::string err) {
    if(err.empty()) {
        Log::debug("tcp server start success");
        //testClient();
        testTcpPool();
    } else {
        Log::error("tcp server start failed: %s ", err.c_str());
    }
}

void testServer()
{
    CNet* net = CNet::Create();
    CTcpServer* svr = CTcpServer::Create(net, OnTcpConnection);
    svr->OnClose = OnTcpSvrClose;
    svr->OnError = OnTcpError;
    svr->OnListen = OnTcpListen;
    svr->Listen("0.0.0.0", svrport);
}

//////////////////////////////////////////////////////////////////////////

static void OnHttpRequest(Http::CHttpServer *server, Http::CIncomingMessage *request, Http::CHttpResponse *response) {
    Log::debug("%d %s %d", request->method, request->url.c_str(), request->version);
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
    CNet* net = CNet::Create();
    Http::CHttpServer *svr = Http::CHttpServer::Create(net);
    svr->OnRequest = OnHttpRequest;
    svr->Listen("0.0.0.0", svrport);
}

//////////////////////////////////////////////////////////////////////////

int _tmain(int argc, _TCHAR* argv[])
{
    Log::open(Log::Print::both, Log::Level::debug, "./log.txt");

    //testServer();
    testHttpServer();

	Sleep(INFINITE);
	return 0;
}

