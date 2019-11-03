#include "uvnettcp.h"
#include "Log.h"

namespace uvNetPlus {

bool net_is_ipv4(const char* input) {
    struct sockaddr_in addr;

    if (!input) return false;
    if (!strchr(input, '.')) return false;
    if (uv_inet_pton(AF_INET, input, &addr.sin_addr) != 0) return false;

    return true;
}

bool net_is_ipv6(const char* input) {
    struct sockaddr_in6 addr;

    if (!input) return false;
    if (!strchr(input, '.')) return false;
    if (uv_inet_pton(AF_INET6, input, &addr.sin6_addr) != 0) return false;

    return true;
}

int net_is_ip(const char* input) {
    if(net_is_ipv4(input))
        return 4;
    if(net_is_ipv6(input))
        return 6;
    return 0;
}

//////////////////////////////////////////////////////////////////////////

#define READY_CALLBACK      if(OnReady) OnReady(this);
#define ERROR_CALLBACK(e)   if(OnError) OnError(this,e);
#define CONNECT_CALLBACK(e) if(OnConnect) OnConnect(this,e);

static void on_uv_close(uv_handle_t* handle) {
    CUNTcpSocket *skt = (CUNTcpSocket*)handle->data;
    Log::warning("close client %s  %d", skt->m_strRemoteIP.c_str(), skt->m_nRemotePort);
    if(skt->OnCLose) 
        skt->OnCLose(skt);
    skt->m_bInit = false;

    if(skt->m_bUserClose)
        delete skt;
}

static void on_uv_shutdown(uv_shutdown_t* req, int status) {
    CUNTcpSocket *skt = (CUNTcpSocket*)req->data;
     Log::warning("shutdown client %s  %d", skt->m_strRemoteIP.c_str(), skt->m_nRemotePort);
    delete req;
    uv_close((uv_handle_t*)&skt->uvTcp, on_uv_close);
}

static void on_uv_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
    CUNTcpSocket *skt = (CUNTcpSocket*)handle->data;
    *buf = uv_buf_init(skt->readBuff, 1024*1024);
}

static void on_uv_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    CUNTcpSocket *skt = (CUNTcpSocket*)stream->data;
    if(nread < 0) {
        skt->m_bConnect = false;
        if(nread == UV__ECONNRESET || nread == UV_EOF) {
            //对端发送了FIN
            if(skt->OnEnd) 
                skt->OnEnd(skt);
            uv_close((uv_handle_t*)&skt->uvTcp, on_uv_close);
        } else {
            uv_shutdown_t* req = new uv_shutdown_t;
            req->data = skt;
            printf("Read error %s", uv_strerror((int)nread));
            if(skt->OnError) 
                skt->OnError(skt, uv_strerror((int)nread));
            uv_shutdown(req, stream, on_uv_shutdown);
        }
        return;
    }
    skt->bytesRead += (uint32_t)nread;
    if (skt->OnRecv)   //数据接收回调
        skt->OnRecv(skt, buf->base, (int)nread);
}

static void on_uv_connect(uv_connect_t* req, int status){
    CUNTcpSocket* skt = (CUNTcpSocket*)req->data;
    delete req;

    if(status != 0){
        if(skt->OnConnect)
            skt->OnConnect(skt, uv_strerror(status));
        return;
    }

    skt->m_bConnect = true;

    // 自动开始接收数据
    if(skt->autoRecv){
        int ret = uv_read_start((uv_stream_t*)&skt->uvTcp, on_uv_alloc, on_uv_read);
        if(ret != 0){
            if(skt->OnError)
                skt->OnError(skt, uv_strerror(ret));
        }
    }

    // 连接完成回调
    if(skt->OnConnect)
        skt->OnConnect(skt, "");

    // 连接成功前缓存的需要发送的数据，将其发送
    skt->syncSend();
}

/** 数据发送完成 */
static void on_uv_write(uv_write_t* req, int status) {
    CUNTcpSocket* skt = (CUNTcpSocket*)req->data;
    //printf("write finish %d", status);
    if(status != 0) {
        if(skt->OnError)
            skt->OnError(skt, uv_strerror(status));
    }

    if(skt->copy){
        for (auto it : skt->sendingList) {
            free(it.base);
        }
    }
    skt->sendingList.clear();

    bool empty = false;
    uv_mutex_lock(&skt->sendMtx);
    empty = skt->sendList.empty();
    uv_mutex_unlock(&skt->sendMtx);

    if(empty){
        if(skt->OnDrain)
            skt->OnDrain(skt);
    } else {
        skt->syncSend();
    }
}

CTcpSocket::CTcpSocket()
    : OnReady(nullptr)
    , OnConnect(nullptr)
    , OnRecv(nullptr)
    , OnDrain(nullptr)
    , OnCLose(nullptr)
    , OnEnd(nullptr)
    , OnTimeout(nullptr)
    , OnError(nullptr)
    , autoRecv(true)
    , copy(true)
    , userData(NULL)
{}

CTcpSocket::~CTcpSocket(){}

CTcpSocket* CTcpSocket::Create(CNet* net, void *usr, bool copy){
    CUNTcpSocket* ret = new CUNTcpSocket((CUVNetPlus*)net, copy);
    ret->userData = usr;
    ret->copy = copy;
    return ret;
}

CUNTcpSocket::CUNTcpSocket(CUVNetPlus* net, bool copy)
    : m_pNet(net)
    , m_pSvr(nullptr)
    , m_bSetLocal(false)
    , m_bInit(false)
    , m_bConnect(false)
    , bytesRead(0)
    , m_bUserClose(false)
{
    readBuff = (char *)calloc(1, 1024*1024);
    uv_mutex_init(&sendMtx);
}

CUNTcpSocket::~CUNTcpSocket()
{
    free(readBuff);
    uv_mutex_destroy(&sendMtx);
    if(m_pSvr) {
        m_pSvr->removeClient(this);
    }
}

void CUNTcpSocket::Delete()
{
    OnReady = nullptr;
    OnConnect = nullptr;
    OnRecv = nullptr;
    OnDrain = nullptr;
    OnCLose = nullptr;
    OnEnd = nullptr;
    OnTimeout = nullptr;
    OnError = nullptr;
    m_pNet->AddEvent(ASYNC_EVENT_TCP_CLTCLOSE, this);
}

void CUNTcpSocket::syncInit()
{
    uvTcp.data = this;
    int ret = uv_tcp_init(&m_pNet->pNode->m_uvLoop, &uvTcp);
    if(ret != 0) {
        ERROR_CALLBACK(uv_strerror(ret));
        return;
    }
    m_bInit = true;
    READY_CALLBACK; // socket 建立完成回调
}

void CUNTcpSocket::syncConnect()
{
    syncInit();
    int ret = 0;
    int t = net_is_ip(m_strRemoteIP.c_str());
    uv_connect_t *req = new uv_connect_t;
    req->data = this;
    if(t == 4){
        struct sockaddr_in addr;
        ret = uv_ip4_addr(m_strRemoteIP.c_str(), m_nRemotePort, &addr);
        ret = uv_tcp_connect(req, &uvTcp, (struct sockaddr*)&addr, on_uv_connect);
        if(ret != 0) {
            CONNECT_CALLBACK(uv_strerror(ret));
            delete req;
        }
    } else if(t == 6){
        struct sockaddr_in6 addr;
        ret = uv_ip6_addr(m_strRemoteIP.c_str(), m_nRemotePort, &addr);
        ret = uv_tcp_connect(req, &uvTcp, (struct sockaddr*)&addr, on_uv_connect);
        if(ret != 0) {
            CONNECT_CALLBACK(uv_strerror(ret));
            delete req;
        }
    } else {
        if(ret != 0) {
            CONNECT_CALLBACK("ip 不合法");
            delete req;
        }
    }
}

void CUNTcpSocket::syncSend()
{
    uv_mutex_lock(&sendMtx);
    size_t num = sendList.size();
    uv_buf_t *bufs = NULL;
    if(num > 0) {
        bufs = new uv_buf_t[num];
        int i =0;
        for (auto &it:sendList)
        {
            //Log::debug(it.base);
            bufs[i++] = it;
            sendingList.push_back(it);
        }
        sendList.clear();
    }
    uv_mutex_unlock(&sendMtx);
    if(num == 0)    //没有需要发送的数据
        return;

    uv_write_t* req = new uv_write_t;
    req->data = this;
    int ret = uv_write(req, (uv_stream_t*)&uvTcp, bufs, (unsigned int)num, on_uv_write);
    if(ret != 0){
        ERROR_CALLBACK(uv_strerror(ret));
    }
    delete[] bufs;
}

void CUNTcpSocket::syncClose()
{
    m_bUserClose = true;
    if(m_bInit && m_bConnect) {
        uv_shutdown_t* req = new uv_shutdown_t;
        req->data = this;
        uv_shutdown(req, (uv_stream_t*)&uvTcp, on_uv_shutdown);
    } else if(!m_bInit){
        delete this;
    }
}

void CUNTcpSocket::Connect(std::string strIP, uint32_t nPort)
{
    m_strRemoteIP = strIP;
    m_nRemotePort = nPort;
    m_pNet->AddEvent(ASYNC_EVENT_TCP_CONNECT, this);
}

void CUNTcpSocket::SetLocal(std::string strIP, uint32_t nPort)
{
    m_strLocalIP = strIP;
    m_nLocalPort = nPort;
    m_bSetLocal = true;
}

void CUNTcpSocket::Send(const char *pData, uint32_t nLen)
{
    char* tmp = NULL;
    if(copy) {
        tmp = (char*)malloc(nLen);
        memcpy(tmp, pData, nLen);
    } else {
        tmp = (char*)pData;
    }
    uv_mutex_lock(&sendMtx);
    sendList.push_back(uv_buf_init(tmp, nLen));
    uv_mutex_unlock(&sendMtx);

    if(m_bConnect || m_pSvr) //客户端需要建立连接完成才能发送，服务端直接就能发送
        m_pNet->AddEvent(ASYNC_EVENT_TCP_SEND, this);
}

//////////////////////////////////////////////////////////////////////////

#define LISTEN_CALLBACK(e) if(OnListen) OnListen(this, e);
#define CONNECTION_CALLBACK(e,c) if(OnConnection) OnConnection(this, e, c);

static void on_server_close(uv_handle_t* handle) {
    CUNTcpServer *skt = (CUNTcpServer*)handle->data;
    // printf("close client %s  %d\n", skt->client->m_strRemoteIP.c_str(), skt->remotePort);
    if(skt->OnClose) 
        skt->OnClose(skt, "");
    skt->m_bListening = false;
    delete skt;
}

static void on_connection(uv_stream_t* server, int status) {
    CUNTcpServer *svr = (CUNTcpServer*)server->data;
    svr->syncConnection(server, status);
}

CTcpServer::CTcpServer()
    : userData(nullptr)
    , OnListen(nullptr)
    , OnConnection(nullptr)
    , OnClose(nullptr)
    , OnError(nullptr)
{}

CTcpServer::~CTcpServer(){}

CTcpServer* CTcpServer::Create(CNet* net, ConnCB onConnection, void *usr){
    CUNTcpServer *ret = new CUNTcpServer((CUVNetPlus*)net);
    ret->OnConnection = onConnection;
    ret->userData = usr;
    return ret;
}

CUNTcpServer::CUNTcpServer(CUVNetPlus* net)
    : m_pNet(net)
    , m_nBacklog(512)
    , m_bListening(false)
{}

CUNTcpServer::~CUNTcpServer()
{
}

void CUNTcpServer::Delete()
{
    OnListen = nullptr;
    OnConnection = nullptr;
    OnClose = nullptr;
    OnError = nullptr;
    m_pNet->AddEvent(ASYNC_EVENT_TCP_SVRCLOSE, this);
}

bool CUNTcpServer::Listen(std::string strIP, uint32_t nPort)
{
    if(m_bListening){
        Log::error("this server is already on listening");
        return false;
    }

    m_nFamily = net_is_ip(strIP.c_str());
    if(m_nFamily != 4 && m_nFamily != 6) {
        Log::error("listen an invalid ip");
        return false;
    }

    m_strLocalIP = strIP;
    m_nLocalPort = nPort;
    m_bListening = true;
    m_pNet->AddEvent(ASYNC_EVENT_TCP_LISTEN, this);
    return true;
}

bool CUNTcpServer::Listening() {
    return m_bListening;
}

void CUNTcpServer::syncListen()
{
    uvTcp.data = this;
    uv_tcp_init(&m_pNet->pNode->m_uvLoop, &uvTcp);

    int ret = 0;
    if(m_nFamily == 4) {
        struct sockaddr_in addr;
        ret = uv_ip4_addr(m_strLocalIP.c_str(), m_nLocalPort, &addr);
        ret = uv_tcp_bind(&uvTcp, (struct sockaddr*)&addr, 0);
    } else if(m_nFamily == 6) {
        struct sockaddr_in6 addr;
        ret = uv_ip6_addr(m_strLocalIP.c_str(), m_nLocalPort, &addr);
        ret = uv_tcp_bind(&uvTcp, (struct sockaddr*)&addr, 0);
    } else {
        m_bListening = false;
        if(OnListen) OnListen(this, "listen an invalid ip");
        return;
    }

    ret = uv_listen((uv_stream_t*)&uvTcp, m_nBacklog, on_connection);
    if(ret != 0) {
        m_bListening = false;
        if(OnListen) OnListen(this, uv_strerror(ret));
    } else {
        m_bListening = true;
        if(OnListen) OnListen(this, "");
    }
}

void CUNTcpServer::syncConnection(uv_stream_t* server, int status)
{
    if (status != 0) {
        CONNECTION_CALLBACK(uv_strerror(status), nullptr);
        return;
    }
    CUNTcpServer *svr = (CUNTcpServer*)server->data;

    CUNTcpSocket *client = new CUNTcpSocket(m_pNet);
    client->m_pSvr = this;
    client->syncInit();
    int ret = uv_accept(server, (uv_stream_t*)(&client->uvTcp));
    if (ret != 0) {
        CONNECTION_CALLBACK(uv_strerror(ret), nullptr);
        return;
    }

    // socket本地ip和端口
    struct sockaddr sockname;
    int namelen = sizeof(struct sockaddr);
    ret = uv_tcp_getsockname(&client->uvTcp, &sockname, &namelen);
    if(sockname.sa_family == AF_INET) {
        struct sockaddr_in* sin = (struct sockaddr_in*)&sockname;
        char addr[46] = {0};
        uv_ip4_name(sin, addr, 46);
        client->m_strLocalIP = addr;
        client->m_nLocalPort = sin->sin_port;
    } else if(sockname.sa_family == AF_INET6) {
        struct sockaddr_in6* sin6 = (struct sockaddr_in6*)&sockname;
        char addr[46] = {0};
        uv_ip6_name(sin6, addr, 46);
        client->m_strLocalIP = addr;
        client->m_nLocalPort = sin6->sin6_port;
    }

    //socket远端ip和端口
    struct sockaddr peername;
    namelen = sizeof(struct sockaddr);
    ret = uv_tcp_getpeername(&client->uvTcp, &peername, &namelen);
    if(peername.sa_family == AF_INET) {
        struct sockaddr_in* sin = (struct sockaddr_in*)&peername;
        char addr[46] = {0};
        uv_ip4_name(sin, addr, 46);
        client->m_strRemoteIP = addr;
        client->m_nRemotePort = sin->sin_port;
    } else if(peername.sa_family == AF_INET6) {
        struct sockaddr_in6* sin6 = (struct sockaddr_in6*)&peername;
        char addr[46] = {0};
        uv_ip6_name(sin6, addr, 46);
        client->m_strRemoteIP = addr;
        client->m_nRemotePort = sin6->sin6_port;
    }

    m_listClients.push_back(client);
    CONNECTION_CALLBACK("", client);

    uv_read_start((uv_stream_t*)&client->uvTcp, on_uv_alloc, on_uv_read);
}

void CUNTcpServer::syncClose()
{

    for(auto c:m_listClients){
        c->Delete();
    }
    m_bListening = false;
}

void CUNTcpServer::removeClient(CUNTcpSocket* c)
{
    m_listClients.remove(c);
    if(!m_bListening && m_listClients.empty()){
        uv_close((uv_handle_t*)&uvTcp, on_server_close);
    }
}

//////////////////////////////////////////////////////////////////////////

}