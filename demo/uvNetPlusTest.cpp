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
    clientData* data = (clientData*)skt->UserData();
    Log::debug("client %d ready", data->tid);
}

void OnClientConnect(CTcpClient* skt, string err){
    clientData* data = (clientData*)skt->UserData();
    if(err.empty()){
        char buff[1024] = {0};
        sprintf(buff, "client%d %d",data->tid, data->finishNum+1);
        skt->Send(buff, strlen(buff));
    } else {
        Log::error("client%d connect err: %s", data->tid, err.c_str());
        skt->Delete();
    }
}

void OnClientRecv(CTcpClient* skt, char *data, int len){
    clientData* c = (clientData*)skt->UserData();
    c->finishNum++;
    Log::debug("client%d recv[%d]: %s", c->tid, c->finishNum, data);
    //skt->Delete();
    if(c->finishNum < c->tid) {
        char buff[1024] = {0};
        sprintf(buff, "client%d %d",c->tid, c->finishNum+1);
        skt->Send(buff, strlen(buff));
    } else {
        skt->Delete();
    }
}

void OnClientDrain(CTcpClient* skt){
    clientData* data = (clientData*)skt->UserData();
    Log::debug("client%d drain", data->tid);
}

void OnClientClose(CTcpClient* skt){
    clientData* data = (clientData*)skt->UserData();
    Log::debug("client%d close", data->tid);
    delete data;
}

void OnClientEnd(CTcpClient* skt){
    clientData* data = (clientData*)skt->UserData();
    Log::debug("client%d end", data->tid);
}

void OnClientError(CTcpClient* skt, string err){
    clientData* data = (clientData*)skt->UserData();
    Log::error("client%d error: %s ", data->tid, err.c_str());
}

void testClient()
{
    CNet* net = CNet::Create();
    for (int i=0; i<1000; i++) {
        clientData* data = new clientData;
        data->tid = i+1;
        data->finishNum = 0;
        CTcpClient* client = CTcpClient::Create(net, OnClientReady, (void*)data);
        client->HandleRecv(OnClientRecv);
        client->HandleDrain(OnClientDrain);
        client->HandleClose(OnClientClose);
        client->HandleEnd(OnClientEnd);
        client->HandleError(OnClientError);
        client->Connect("127.0.0.1", svrport, OnClientConnect);
    }
}

//////////////////////////////////////////////////////////////////////////
/////////////     tcp客户端连接池      ///////////////////////////////////

static void OnConnectRequest(TcpRequest* req, std::string error){
    clientData* data = (clientData*)req->usr;
    if(!error.empty())
        Log::error("OnConnectRequest client%d error: %s ", data->tid, error.c_str());
}

static void OnConnectResponse(TcpRequest* req, std::string error, const char *data, int len){
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
        client->MaxConns(10);
        client->MaxIdle(10);
        for (int i=0; i<500; i++) {
            Log::debug("new request %d", i);
            clientData* data = new clientData;
            data->tid = i+1;
            data->finishNum = 0;
            client->Request("127.0.0.1", svrport, "123456789", 9, data, true, true);
        }
    });
    t.detach();
}

//////////////////////////////////////////////////////////////////////////

struct sclientData {
    int id;
};

void OnSvrCltRecv(CTcpClient* skt, char *data, int len){
    sclientData *c = (sclientData*)skt->UserData();
    Log::debug("server client%d recv: %s ", c->id, data);
    char buff[1024] = {0};
    sprintf(buff, "server client%d answer",c->id);
    skt->Send(buff, strlen(buff));
}

void OnSvrCltDrain(CTcpClient* skt){
    sclientData *c = (sclientData*)skt->UserData();
    Log::debug("server client%d drain", c->id);
}

void OnSvrCltClose(CTcpClient* skt){
    sclientData *c = (sclientData*)skt->UserData();
    Log::debug("server client%d close", c->id);
    delete c;
}

void OnSvrCltEnd(CTcpClient* skt){
    sclientData *c = (sclientData*)skt->UserData();
    Log::debug("server client%d end", c->id);
}

void OnSvrCltError(CTcpClient* skt, string err){
    sclientData *c = (sclientData*)skt->UserData();
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
    client->SetUserData(c);
    client->HandleRecv(OnSvrCltRecv);
    client->HandleDrain(OnSvrCltDrain);
    client->HandleClose(OnSvrCltClose);
    client->HandleEnd(OnSvrCltEnd);
    client->HandleError(OnSvrCltError);
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
    svr->HandleClose(OnTcpSvrClose);
    svr->HandleError(OnTcpError);
    svr->Listen("0.0.0.0", svrport, OnTcpListen);
}

//////////////////////////////////////////////////////////////////////////

int _tmain(int argc, _TCHAR* argv[])
{
    Log::open(Log::Print::both, Log::Level::debug, "./log.txt");

    testServer();

	Sleep(INFINITE);
	return 0;
}

