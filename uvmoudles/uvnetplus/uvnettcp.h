#pragma once
#include "uvnetplus.h"
#include "uvnetprivate.h"

class CUNTcpServer;
//////////////////////////////////////////////////////////////////////////

class CUNTcpClient : public uvNetPlus::CTcpClient
{
public:
    CUNTcpClient(CUVNetPlus* net, fnOnTcpEvent onReady, void *usr);
    ~CUNTcpClient();
    virtual void Delete();
    void syncInit();
    void syncConnect();
    void syncSend();
    void syncClose();
    virtual void Connect(std::string strIP, uint32_t nPort, fnOnTcpError onConnect);
    virtual void SetLocal(std::string strIP, uint32_t nPort);
    virtual void HandleRecv(fnOnTcpRecv onRecv);
    virtual void HandleDrain(fnOnTcpEvent onDrain);
    virtual void HandleClose(fnOnTcpEvent onClose);
    virtual void HandleEnd(fnOnTcpEvent onEnd);
    virtual void HandleTimeOut(fnOnTcpEvent onTimeOut);
    virtual void HandleError(fnOnTcpError onError);
    virtual void Send(char *pData, uint32_t nLen);
    virtual void SetUserData(void* usr){m_pData = usr;};
    virtual void* UserData(){return m_pData;};

public:
    CUVNetPlus       *m_pNet;      //事件线程句柄
    CUNTcpServer     *m_pSvr;      //客户端实例为null，服务端实例指向监听服务句柄
    uv_tcp_t          uvTcp;
    void             *m_pData;

    string            m_strRemoteIP; //远端ip
    uint32_t          m_nRemotePort; //远端端口
    string            m_strLocalIP;  //本地ip
    uint32_t          m_nLocalPort;  //本地端口
    bool              m_bSetLocal;   //作为客户端时，是否设置本地绑定信息
    bool              m_bInit;


    fnOnTcpEvent      m_funOnReady;     //socket创建完成
    fnOnTcpError      m_funOnConnect;   //连接完成
    fnOnTcpRecv       m_funOnRecv;      //收到数据
    fnOnTcpEvent      m_funOnDrain;     //发送队列全部完成
    fnOnTcpEvent      m_funOnCLose;     //socket关闭
    fnOnTcpEvent      m_funOnEnd;       //收到对方fin,读到eof
    fnOnTcpEvent      m_funOnTimeout;   //超时回调
    fnOnTcpError      m_funOnError;     //错误回调

    char             *readBuff;         // 接收缓存
    uint32_t          bytesRead;        // 统计累计接收大小
    list<uv_buf_t>    sendList;         // 发送缓存
    list<uv_buf_t>    sendingList;      // 正在发送
    uv_mutex_t        sendMtx;          // 发送锁
};

//////////////////////////////////////////////////////////////////////////

class CUNTcpServer : public uvNetPlus::CTcpServer
{
public:
    CUNTcpServer(CUVNetPlus* net, fnOnTcpConnection onConnection, void *usr);
    ~CUNTcpServer();
    virtual void Delete();
    void syncListen();
    void syncConnection(uv_stream_t* server, int status);
    void syncClose();
    virtual void Listen(std::string strIP, uint32_t nPort, fnOnTcpSvr onListion);
    virtual void HandleClose(fnOnTcpSvr onClose);
    virtual void HandleError(fnOnTcpSvr onError);
    virtual void* UserData(){return m_pData;};
    void removeClient(CUNTcpClient* c);

public:
    CUVNetPlus       *m_pNet;
    uv_tcp_t          uvTcp;
    void             *m_pData;

    string            m_strLocalIP;
    uint32_t          m_nLocalPort;
    int               m_nBacklog;       //syns queue的大小，默认为512
    bool              m_bInit;          //开始监听时true，不在监听时为false

    fnOnTcpSvr          m_funOnListen;
    fnOnTcpConnection   m_funOnConnection;
    fnOnTcpSvr          m_funOnClose;
    fnOnTcpSvr          m_funOnError;

    list<CUNTcpClient*> m_listClients;
};

