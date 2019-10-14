#pragma once
#include "uvnetpuclic.h"
#include <string>
#include <stdint.h>

namespace uvNetPlus {

/** 事件循环eventloop执行线程，封装uv_loop */
class CNet
{
public:
    static CNet* Create();
    virtual ~CNet(){};
};

//////////////////////////////////////////////////////////////////////////

/** TCP客户端 */
class CTcpClient
{
    typedef void (*EventCB)(CTcpClient* skt);
    typedef void (*RecvCB)(CTcpClient* skt, char *data, int len);
    typedef void (*ErrorCB)(CTcpClient* skt, std::string error);
public:
    EventCB      OnReady;     //socket创建完成
    ErrorCB      OnConnect;   //连接完成
    RecvCB       OnRecv;      //收到数据
    EventCB      OnDrain;     //发送队列全部完成
    EventCB      OnCLose;     //socket关闭
    EventCB      OnEnd;       //收到对方fin,读到eof
    EventCB      OnTimeout;   //超时回调
    ErrorCB      OnError;     //错误回调

    bool         autoRecv;    //连接建立后是否立即自动接收数据。默认true
    bool         copy;        //发送的数据拷贝到临时区域
    void        *userData;    //用户绑定自定义数据

    /**
     * 创建一个tcp连接客户端实例
     * @param net 环境句柄
     * @param usr 设定用户自定义数据
     * @param copy 调用发送接口时，是否将数据拷贝到缓存由内部进行管理
     */
    static CTcpClient* Create(CNet* net, void *usr=nullptr, bool copy=true);

    /**
     * 异步删除这个实例
     */
    virtual void Delete() = 0;

    /**
     * 连接服务器，连接完成后调用OnConnect回调
     */
    virtual void Connect(std::string strIP, uint32_t nPort) = 0;

    /**
     * 设置socket的本地端口，如果不指定，将有系统自动分配
     * @param strIP 本地IP，用来指定本定使用哪一个网卡。空表示不指定。
     * @param nPort 本定端口，0表示不指定
     */
    virtual void SetLocal(std::string strIP, uint32_t nPort) = 0; 

    /**
     * 发送数据。将数据放到本地缓存起来
     */
    virtual void Send(const char *pData, uint32_t nLen) = 0;
protected:
    CTcpClient();
    virtual ~CTcpClient() = 0;
};

/** TCP服务端 */
class CTcpServer
{
    typedef void (*EventCB)(CTcpServer* svr, std::string err);
    typedef void (*ConnCB)(CTcpServer* svr, std::string err, CTcpClient* client);
public:

    EventCB          OnListen;       // 开启监听完成回调，错误时上抛错误消息
    ConnCB           OnConnection;   // 新连接回调
    EventCB          OnClose;        // 监听socket关闭完成回调
    EventCB          OnError;        // 发生错误回调

    void            *userData;

    /**
     * 创建一个tcp服务端实例
     * @param net 环境句柄
     * @param onConnection 指定收到新连接时的回调
     * @param usr 设定用户自定义数据
     */
    static CTcpServer* Create(CNet* net, ConnCB onConnection, void *usr=nullptr);

    /**
     * 异步删除当前实例
     */
    virtual void Delete() = 0;

    /**
     * 启动监听
     * @param strIP 本地IP，用来指定本定使用哪一个网卡
     * @param nPort 本地监听端口
     */
    virtual bool Listen(std::string strIP, uint32_t nPort) = 0;

    /** 服务器是否在监听连接 */
    virtual bool Listening() = 0;
protected:
    CTcpServer();
    virtual ~CTcpServer() = 0;
};

//////////////////////////////////////////////////////////////////////////

/** TCP连接池 请求结构 */
class CTcpRequest {
public:
    std::string     host;   //请求目标域名或ip
    uint32_t        port;   //请求端口
    std::string     localaddr; //本地ip，表明使用哪一块网卡。默认空，不限制
    void           *usr;    //用户自定义数据
    bool            copy;   //需要发送的数据是否拷贝到内部维护
    bool            recv;   //tcp请求是否需要接收数据

    /* 向请求追加发送数据,在发送回调中使用,不能另开线程 */
    virtual void Request(const char* buff, int length) = 0;

    /**
     * 一个请求完成，将socket放到空闲池里面去
     */
    virtual void Finish() = 0;

protected:
    CTcpRequest(){};
    virtual ~CTcpRequest(){};
};

/** TCP连接池，进行请求应答 */
class CTcpConnPool
{
    typedef void (*ReqCB)(CTcpRequest* req, std::string error);
    typedef void (*ResCB)(CTcpRequest* req, std::string error, const char *data, int len);
public:
    uint32_t   maxConns;    //最大连接数 默认512(busy+idle)
    uint32_t   maxIdle;     //最大空闲连接数 默认100
    uint32_t   timeOut;     //空闲连接超时时间 秒 默认20s 0为永不超时

    ReqCB      OnRequest;
    ResCB      OnResponse;

    /**
     * 创建连接池
     * @param net loop实例
     * @param onReq 发送请求结果回调
     * @param onRes 应答回调
     */
    static CTcpConnPool* Create(CNet* net, ReqCB onReq, ResCB onRes);

    /**
     * 异步删除连接池
     */
    virtual void Delete() = 0;

    /**
     * 发送一个请求
     * @param host 请求目标域名或端口
     * @param port 请求目标端口
     * @param localaddr 本地ip，指定网卡，为空表示不指定
     * @param usr 绑定一个用户数据，回调时作为参数输出
     * @param copy 发送的数据是否拷贝到内部
     * @param recv 是否需要接收应答
     * @return 返回新的请求实例
     */
    virtual CTcpRequest* Request(std::string host, uint32_t port, std::string localaddr, void *usr=nullptr, bool copy=true, bool recv=true) = 0;

protected:
    CTcpConnPool();
    virtual ~CTcpConnPool() = 0;
};

//////////////////////////////////////////////////////////////////////////


}

