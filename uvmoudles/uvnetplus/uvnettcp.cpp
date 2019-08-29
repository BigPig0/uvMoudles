#include "uvnettcp.h"

static bool net_is_ipv4(const char* input) {
    struct sockaddr_in addr;

    if (!input) return false;
    if (!strchr(input, '.')) return false;
    if (uv_inet_pton(AF_INET, input, &addr.sin_addr) != 0) return false;

    return true;
}

static bool net_is_ipv6(const char* input) {
    struct sockaddr_in6 addr;

    if (!input) return false;
    if (!strchr(input, '.')) return false;
    if (uv_inet_pton(AF_INET6, input, &addr.sin6_addr) != 0) return false;

    return true;
}

static int net_is_ip(const char* input) {
    if(net_is_ipv4(input))
        return 4;
    if(net_is_ipv6(input))
        return 6;
    return 0;
}

//////////////////////////////////////////////////////////////////////////

#define READY_CALLBACK if(m_funOnReady) m_funOnReady(this);
#define ERROR_CALLBACK(e) if(m_funOnError) m_funOnError(this,e);
#define CONNECT_CALLBACK(e) if(m_funOnConnect) m_funOnConnect(this,e);

static void on_uv_close(uv_handle_t* handle) {
    CUNTcpClient *skt = (CUNTcpClient*)handle->data;
    // printf("close client %s  %d\n", skt->client->m_strRemoteIP.c_str(), skt->remotePort);
    if(skt->m_funOnCLose) 
        skt->m_funOnCLose(skt);
    skt->m_bInit = false;
    delete skt;
}

static void on_uv_shutdown(uv_shutdown_t* req, int status) {
    CUNTcpClient *skt = (CUNTcpClient*)req->data;
    delete req;
    uv_close((uv_handle_t*)&skt->uvTcp, on_uv_close);
}

static void on_uv_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
    CUNTcpClient *skt = (CUNTcpClient*)handle->data;
    *buf = uv_buf_init(skt->readBuff, 1024*1024);
}

static void on_uv_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    CUNTcpClient *skt = (CUNTcpClient*)stream->data;
    if(nread < 0) {
        if(nread == UV__ECONNRESET || nread == UV_EOF) {
            //对端发送了FIN
            if(skt->m_funOnEnd) 
                skt->m_funOnEnd(skt);
            uv_close((uv_handle_t*)&skt->uvTcp, on_uv_close);
        } else {
            uv_shutdown_t* req = new uv_shutdown_t;
            req->data = skt;
            printf("Read error %s\n", uv_strerror(nread));
            if(skt->m_funOnError) 
                skt->m_funOnError(skt, uv_strerror(nread));
            uv_shutdown(req, stream, on_uv_shutdown);
        }
        return;
    }
    skt->bytesRead += nread;
    if (skt->m_funOnRecv)   //数据接收回调
        skt->m_funOnRecv(skt, buf->base, nread);
}

static void on_uv_connect(uv_connect_t* req, int status){
    CUNTcpClient* skt = (CUNTcpClient*)req->data;
    delete req;

    if(status != 0){
        if(skt->m_funOnConnect)
            skt->m_funOnConnect(skt, uv_strerror(status));
        return;
    }
    int ret = uv_read_start((uv_stream_t*)&skt->uvTcp, on_uv_alloc, on_uv_read);
    if(ret != 0){
        if(skt->m_funOnError)
            skt->m_funOnError(skt, uv_strerror(ret));
    }
    if(skt->m_funOnConnect) // 连接完成回调
        skt->m_funOnConnect(skt, "");
}

/** 数据发送完成 */
static void on_uv_write(uv_write_t* req, int status) {
    CUNTcpClient* skt = (CUNTcpClient*)req->data;
    //printf("write finish %d\n", status);
    if(status != 0) {
        if(skt->m_funOnError)
            skt->m_funOnError(skt, uv_strerror(status));
    }

    for (auto it : skt->sendingList) {
        free(it.base);
    }
    skt->sendingList.clear();

    bool empty = false;
    uv_mutex_lock(&skt->sendMtx);
    empty = skt->sendList.empty();
    uv_mutex_unlock(&skt->sendMtx);

    if(empty){
        if(skt->m_funOnDrain)
            skt->m_funOnDrain(skt);
    } else {
        skt->syncSend();
    }
}

CUNTcpClient::CUNTcpClient(CUVNetPlus* net, fnOnTcpEvent onReady, void *usr)
    : m_pNet(net)
    , m_pSvr(nullptr)
    , m_bSetLocal(false)
    , m_bInit(false)
    , m_pData(usr)
    , m_funOnReady(onReady)
    , m_funOnConnect(nullptr)
    , m_funOnRecv(nullptr)
    , m_funOnDrain(nullptr)
    , m_funOnCLose(nullptr)
    , m_funOnEnd(nullptr)
    , m_funOnTimeout(nullptr)
    , m_funOnError(nullptr)
    , bytesRead(0)
{
    readBuff = (char *)calloc(1, 1024*1024);
    uv_mutex_init(&sendMtx);
}

CUNTcpClient::~CUNTcpClient()
{
    free(readBuff);
    uv_mutex_destroy(&sendMtx);
    if(m_pSvr) {
        m_pSvr->removeClient(this);
    }
}

void CUNTcpClient::Delete()
{
    m_pNet->AddEvent(ASYNC_EVENT_TCP_CLTCLOSE, this);
}

void CUNTcpClient::syncInit()
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

void CUNTcpClient::syncConnect()
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

void CUNTcpClient::syncSend()
{
    uv_mutex_lock(&sendMtx);
    size_t num = sendList.size();
    uv_buf_t *bufs = new uv_buf_t[num];
    int i =0;
    for (auto it:sendList)
    {
        bufs[i++] = it;
        sendingList.push_back(it);
    }
    sendList.clear();
    uv_mutex_unlock(&sendMtx);

    uv_write_t* req = new uv_write_t;
    req->data = this;
    int ret = uv_write(req, (uv_stream_t*)&uvTcp, bufs, num, on_uv_write);
    if(ret != 0){
        ERROR_CALLBACK(uv_strerror(ret));
    }
    delete[] bufs;
}

void CUNTcpClient::syncClose()
{
    if(m_bInit) {
        uv_shutdown_t* req = new uv_shutdown_t;
        req->data = this;
        uv_shutdown(req, (uv_stream_t*)&uvTcp, on_uv_shutdown);
    } else {
        delete this;
    }
}

void CUNTcpClient::Connect(std::string strIP, uint32_t nPort, fnOnTcpError onConnect)
{
    m_strRemoteIP = strIP;
    m_nRemotePort = nPort;
    m_funOnConnect = onConnect;
    m_pNet->AddEvent(ASYNC_EVENT_TCP_CONNECT, this);
    
}

void CUNTcpClient::SetLocal(std::string strIP, uint32_t nPort)
{
    m_strLocalIP = strIP;
    m_nLocalPort = nPort;
    m_bSetLocal = true;
}

void CUNTcpClient::HandleRecv(fnOnTcpRecv onRecv)
{
    m_funOnRecv = onRecv;
}

void CUNTcpClient::HandleDrain(fnOnTcpEvent onDrain)
{
    m_funOnDrain = onDrain;
}

void CUNTcpClient::HandleClose(fnOnTcpEvent onClose)
{
    m_funOnCLose = onClose;
}

void CUNTcpClient::HandleEnd(fnOnTcpEvent onEnd)
{
    m_funOnEnd = onEnd;
}

void CUNTcpClient::HandleTimeOut(fnOnTcpEvent onTimeOut)
{
    m_funOnTimeout = onTimeOut;
}

void CUNTcpClient::HandleError(fnOnTcpError onError)
{
    m_funOnError = onError;
}

void CUNTcpClient::Send(char *pData, uint32_t nLen)
{
    char* tmp = (char*)malloc(nLen);
    memcpy(tmp, pData, nLen);
    uv_mutex_lock(&sendMtx);
    sendList.push_back(uv_buf_init(tmp, nLen));
    uv_mutex_unlock(&sendMtx);
    m_pNet->AddEvent(ASYNC_EVENT_TCP_SEND, this);
}

//////////////////////////////////////////////////////////////////////////

#define LISTEN_CALLBACK(e) if(m_funOnListen) m_funOnListen(this, e);
#define CONNECTION_CALLBACK(e,c) if(m_funOnConnection) m_funOnConnection(this, e, c);

static void on_server_close(uv_handle_t* handle) {
    CUNTcpServer *skt = (CUNTcpServer*)handle->data;
    // printf("close client %s  %d\n", skt->client->m_strRemoteIP.c_str(), skt->remotePort);
    if(skt->m_funOnClose) 
        skt->m_funOnClose(skt, "");
    skt->m_bInit = false;
    delete skt;
}

static void on_connection(uv_stream_t* server, int status) {
    CUNTcpServer *svr = (CUNTcpServer*)server->data;
    svr->syncConnection(server, status);
}

CUNTcpServer::CUNTcpServer(CUVNetPlus* net, fnOnTcpConnection onConnection, void *usr)
    : m_pNet(net)
    , m_nBacklog(512)
    , m_bInit(false)
    , m_pData(usr)
    , m_funOnListen(nullptr)
    , m_funOnConnection(onConnection)
    , m_funOnClose(nullptr)
    , m_funOnError(nullptr)
{
}

CUNTcpServer::~CUNTcpServer()
{
}

void CUNTcpServer::Delete()
{
    m_pNet->AddEvent(ASYNC_EVENT_TCP_SVRCLOSE, this);
}

void CUNTcpServer::syncListen()
{
    uvTcp.data = this;
    uv_tcp_init(&m_pNet->pNode->m_uvLoop, &uvTcp);

    int ret = 0;
    int t = net_is_ip(m_strLocalIP.c_str());
    if(t == 4) {
        struct sockaddr_in addr;
        ret = uv_ip4_addr(m_strLocalIP.c_str(), m_nLocalPort, &addr);
        ret = uv_tcp_bind(&uvTcp, (struct sockaddr*)&addr, 0);
    } else if(t == 6) {
        struct sockaddr_in6 addr;
        ret = uv_ip6_addr(m_strLocalIP.c_str(), m_nLocalPort, &addr);
        ret = uv_tcp_bind(&uvTcp, (struct sockaddr*)&addr, 0);
    } else {
        LISTEN_CALLBACK("ip不合法");
        return;
    }

    ret = uv_listen((uv_stream_t*)&uvTcp, m_nBacklog, on_connection);
    if(ret != 0) {
        LISTEN_CALLBACK(uv_strerror(ret));
    } else {
        LISTEN_CALLBACK("");
    }
}

void CUNTcpServer::syncConnection(uv_stream_t* server, int status)
{
    if (status != 0) {
        CONNECTION_CALLBACK(uv_strerror(status), nullptr);
        return;
    }
    CUNTcpServer *svr = (CUNTcpServer*)server->data;

    CUNTcpClient *client = new CUNTcpClient(m_pNet, NULL, NULL);
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
    m_bInit = false;
}

void CUNTcpServer::Listen(std::string strIP, uint32_t nPort, fnOnTcpSvr onListion)
{
    if(!m_bInit){
        m_strLocalIP = strIP;
        m_nLocalPort = nPort;
        m_funOnListen = onListion;
        m_bInit = true;
        m_pNet->AddEvent(ASYNC_EVENT_TCP_LISTEN, this);
    } else {
        onListion(this, "已经开始监听");
    }
}

void CUNTcpServer::HandleClose(fnOnTcpSvr onClose)
{
    m_funOnClose = onClose;
}

void CUNTcpServer::HandleError(fnOnTcpSvr onError)
{
    m_funOnError = onError;
}

void CUNTcpServer::removeClient(CUNTcpClient* c)
{
    m_listClients.remove(c);
    if(!m_bInit && m_listClients.empty()){
        uv_close((uv_handle_t*)&uvTcp, on_server_close);
    }
}

//////////////////////////////////////////////////////////////////////////

namespace uvNetPlus {
    CTcpClient* CTcpClient::Create(CNet* net, fnOnTcpEvent onReady, void *usr){
        return new CUNTcpClient((CUVNetPlus*)net, onReady, usr);
    }

    CTcpServer* CTcpServer::Create(CNet* net, fnOnTcpConnection onConnection, void *usr){
        return new CUNTcpServer((CUVNetPlus*)net, onConnection, usr);
    }
}