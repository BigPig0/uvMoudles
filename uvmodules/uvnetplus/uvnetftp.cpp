#include "uvnetftp.h"

namespace uvNetPlus {
namespace Ftp {
    const char* szFtpCmd[] = {
        "ABOR", //中断数据连接程序
        "ACCT", //系统特权帐号
        "ALLO", //为服务器上的文件存储器分配字节
        "APPE", //添加文件到服务器同名文件
        "CDUP", //改变服务器上的父目录
        "CWD",  //改变服务器上的工作目录
        "DELE", //删除服务器上的指定文件
        "HELP", //返回指定命令信息
        "LIST", //如果是文件名列出文件信息，如果是目录则列出文件列表
        "MODE", //传输模式（S=流模式，B=块模式，C=压缩模式）
        "MKD",  //在服务器上建立指定目录
        "NLST", //列出指定目录内容
        "NOOP", //无动作，除了来自服务器上的承认
        "PASS", //系统登录密码
        "PASV", //请求服务器等待数据连接
        "PORT", //IP 地址和两字节的端口 ID
        "PWD",  //显示当前工作目录
        "QUIT", //从 FTP 服务器上退出登录
        "REIN", //重新初始化登录状态连接
        "REST", //由特定偏移量重启文件传递
        "RETR", //从服务器上找回（复制）文件
        "RMD",  //在服务器上删除指定目录
        "RNFR", //对旧路径重命名
        "RNTO", //对新路径重命名
        "SITE", //由服务器提供的站点特殊参数
        "SMNT", //挂载指定文件结构
        "STAT", //在当前程序或目录上返回信息
        "STOR", //储存（复制）文件到服务器上
        "STOU", //储存文件到服务器名称上
        "STRU", //数据结构（F=文件，R=记录，P=页面）
        "SYST", //返回服务器使用的操作系统
        "TYPE", //数据类型（A=ASCII，E=EBCDIC，I=binary）
        "USER"  //系统登录的用户名
    };

    //////////////////////////////////////////////////////////////////////////
    CFtpRequest::CFtpRequest()
        : port(21)
        , keepAlive(true)
        , usrData(NULL)
        , autodel(true)
        , path("/")
    {}

    CFtpRequest::~CFtpRequest() {}

    CUNFtpRequest::CUNFtpRequest() {
        uv_mutex_init(&mutex);
    }

    CUNFtpRequest::~CUNFtpRequest() {
        uv_mutex_destroy(&mutex);
    }

    /** 删除实例 */
    void CUNFtpRequest::Delete() {
        delete this;
    }

    /**
     * 改变服务器上的工作目录CWD
     */
    void CUNFtpRequest::ChangeWorkingDirectory(std::string path, ResCB cb) {
        uv_mutex_lock(&mutex);
    }

    /**
     * 获取服务器文件列表NLST
     */
    void CUNFtpRequest::FileList(ResCB cb) {
        uv_mutex_lock(&mutex);
    }

    /**
     * 获取文件信息或文件列表LIST
     */
    void CUNFtpRequest::List(ResCB cb) {
        uv_mutex_lock(&mutex);
        tcpSocket->Send(szFtpCmd[FTP_CMD_LIST], strlen(szFtpCmd[FTP_CMD_LIST]));
    }

    /**
     * 下载文件
     */
    void CUNFtpRequest::Download(string file, ResCB cb) {
        uv_mutex_lock(&mutex);
    }

    /**
     * 上传文件
     */
    void CUNFtpRequest::Upload(string file, char *data, int size, ResCB cb) {
        uv_mutex_lock(&mutex);
    }

    /* 收到的数据处理 */
    void CUNFtpRequest::DoReceive(const char *data, int len) {

    }

    /** 发生错误处理 */
    void CUNFtpRequest::DoError(string err) {

    }

    /** 客户端数据全部发送 */
    void CUNFtpRequest::DoDrain() {

    }

    //////////////////////////////////////////////////////////////////////////
    /** Http客户端环境 */
    static void OnClientRecv(CTcpSocket* skt, char *data, int len){
        CUNFtpRequest* req = (CUNFtpRequest*)skt->userData;
        req->DoReceive(data, len);
    }

    static void OnClientDrain(CTcpSocket* skt){
        //Log::debug("client drain");
        CUNFtpRequest* req = (CUNFtpRequest*)skt->userData;
        req->DoDrain();
    }

    static void OnClientClose(CTcpSocket* skt){
        //Log::debug("client close");
    }

    static void OnClientError(CTcpSocket* skt, string err){
        Log::error("client error: %s ", err.c_str());
        CUNFtpRequest* req = (CUNFtpRequest*)skt->userData;
        req->DoError(err);
    }

    //////////////////////////////////////////////////////////////////////////

    struct FtpConnReq {
        CFtpClient     *client;
        std::string     host;
        int             port;
        std::string     user;
        std::string     pwd;
        void           *usrData;
        CFtpClient::ReqCB cb;
    };

    /**
     * 从连接池获取socket成功
     * @param req 连接池获取socket的请求
     * @param skt 获取到的socket实例
     */
    static void OnPoolSocket(CTcpRequest* req, CTcpSocket* skt) {
        FtpConnReq    *connReq = (FtpConnReq*)req->usr;
        CUNFtpRequest *ftp     = new CUNFtpRequest();

        skt->OnRecv     = OnClientRecv;
        skt->OnDrain    = OnClientDrain;
        skt->OnCLose    = OnClientClose;
        //skt->OnEnd      = OnClientEnd;
        skt->OnError    = OnClientError;
        skt->autoRecv   = true;
        skt->copy       = true;
        skt->userData   = ftp;

        ftp->host      = connReq->host;
        ftp->port      = connReq->port;
        ftp->usrData   = connReq->usrData;
        ftp->tcpSocket = skt;

        if(connReq->cb)
            connReq->cb(ftp, connReq->usrData, "");
        else if(connReq->client && connReq->client->OnRequest)
            connReq->client->OnRequest(ftp, connReq->usrData, "");
        else
            skt->Delete();

        delete connReq;
    }

    //从连接池获取socket失败
    static void OnPoolError(CTcpRequest* req, string error) {
        FtpConnReq *connReq = (FtpConnReq*)req->usr;
        if(connReq->cb)
            connReq->cb(NULL, connReq->usrData, error);
        else if(connReq->client && connReq->client->OnRequest)
            connReq->client->OnRequest(NULL, connReq->client, error);
        delete connReq;
    }

    CFtpClient::CFtpClient(CNet* net, uint32_t maxConns, uint32_t maxIdle, uint32_t timeOut, uint32_t maxRequest)
        : OnRequest(NULL)
    {
        connPool = CTcpConnPool::Create(net, OnPoolSocket);
        connPool->maxConns = maxConns;
        connPool->maxIdle  = maxIdle;
        connPool->timeOut  = timeOut;
        connPool->maxRequest = maxRequest;
        connPool->OnError  = OnPoolError;
    }

    CFtpClient::~CFtpClient() {
        connPool->Delete();
    }

    bool CFtpClient::Request(std::string host, int port, std::string user, std::string pwd, void* usrData /*= NULL*/, ReqCB cb /*= NULL*/) {
        FtpConnReq *req = new FtpConnReq();
        req->client  = this;
        req->host = host;
        req->port = port;
        req->user = user;
        req->pwd  = pwd;
        req->usrData = usrData;
        req->cb   = cb;
        return connPool->Request(host, port, "", req, true, true);
    }
}
}