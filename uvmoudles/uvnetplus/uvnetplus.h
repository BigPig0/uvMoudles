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
public:
    typedef void (*fnOnTcpEvent)(CTcpClient* skt);
    typedef void (*fnOnTcpRecv)(CTcpClient* skt, char *data, int len);
    typedef void (*fnOnTcpError)(CTcpClient* skt, std::string error);

    static CTcpClient* Create(CNet* net, fnOnTcpEvent onReady, void *usr=nullptr, bool copy=true);
    virtual void Delete() = 0;
    virtual void Connect(std::string strIP, uint32_t nPort, fnOnTcpError onConnect) = 0;
    virtual void SetLocal(std::string strIP, uint32_t nPort) = 0; 
    virtual void HandleRecv(fnOnTcpRecv onRecv) = 0;
    virtual void HandleDrain(fnOnTcpEvent onDrain) = 0;
    virtual void HandleClose(fnOnTcpEvent onClose) = 0;
    virtual void HandleEnd(fnOnTcpEvent onEnd) = 0;
    virtual void HandleTimeOut(fnOnTcpEvent onTimeOut) = 0;
    virtual void HandleError(fnOnTcpError onError) = 0;
    virtual void Send(char *pData, uint32_t nLen) = 0;
    virtual void SetUserData(void* usr) = 0;
    virtual void* UserData() = 0;
protected:
    CTcpClient(){};
    virtual ~CTcpClient(){};
};

/** TCP服务端 */
class CTcpServer
{
public:
    typedef void (*fnOnTcpSvr)(CTcpServer* svr, std::string err);
    typedef void (*fnOnTcpConnection)(CTcpServer* svr, std::string err, CTcpClient* client);

    static CTcpServer* Create(CNet* net, fnOnTcpConnection onConnection, void *usr=nullptr);
    virtual void Delete() = 0;
    virtual void Listen(std::string strIP, uint32_t nPort, fnOnTcpSvr onListion) = 0;
    virtual void HandleClose(fnOnTcpSvr onClose) = 0;
    virtual void HandleError(fnOnTcpSvr onError) = 0;
    virtual void* UserData() = 0;
protected:
    CTcpServer(){};
    virtual ~CTcpServer(){};
};

//////////////////////////////////////////////////////////////////////////

/** TCP连接池 请求结构 */
class TcpRequest {
public:
    std::string     host;
    uint32_t        port;
    char           *data;
    int             len;
    void           *usr;
    bool            copy;
    bool            recv;

    /* 向请求追加发送数据,在发送回调中使用,不能另开线程 */
    virtual void Request(const char* buff, int length) = 0;

    /**
     * recv为true时，接收完成，只能在接收回调中使用，必须调用
     * recv为false时，发送完成，只能在发送回调中使用，必须调用
     */
    virtual void Finish() = 0;

protected:
    TcpRequest(){};
    virtual ~TcpRequest(){};
};

/** TCP连接池，进行请求应答 */
class CTcpConnPool
{
public:
    /**
     * 定义发送请求回调
     * @param usr 用户绑定的数据
     * @param error 错误信息，为空时成功
     */
    typedef void (*fnOnConnectRequest)(TcpRequest* req, std::string error);

    /**
     * 定义收到应答的回调
     * @param usr 用户绑定的数据
     * @param error 错误信息，为空时成功
     * @param data 收到的数据,用户需要拷贝走，否则内容会改变
     * @param len 收到的数据的长度
     */
    typedef void (*fnOnConnectResponse)(TcpRequest* req, std::string error, const char *data, int len);

    /**
     * 创建连接池
     * @param net loop实例
     * @param onReq 发送请求结果回调
     * @param onRes 应答回调
     */
    static CTcpConnPool* Create(CNet* net, fnOnConnectRequest onReq, fnOnConnectResponse onRes);

    virtual void Delete() = 0;

    /**
     * 发送一个请求
     * @param ip 请求目标ip
     * @param port 请求目标端口
     * @param data 需要发送的数据
     * @param len 需要发送的数据长度
     * @param usr 绑定一个用户数据，回调时作为参数输出
     * @param copy 发送的数据是否拷贝到内部
     * @param recv 是否需要接收应答
     * @return 返回新的请求实例
     */
    virtual TcpRequest* Request(std::string ip, uint32_t port, const char* data, int len, void *usr=nullptr, bool copy=true, bool recv=true) = 0;

    virtual void MaxConns(uint32_t num) = 0;
    virtual void MaxIdle(uint32_t num) = 0;
protected:
    CTcpConnPool(){};
    virtual ~CTcpConnPool(){};
};

//////////////////////////////////////////////////////////////////////////

/** Http接口封装 */
class Http
{
class ClientRequest;
class IncomingMessage;
class ServerResponse;
class Server;

enum PROTOCOL {
    HTTP = 0,
    HTTPS,
    WS,
    WSS
};

enum METHOD {
    OPTIONS = 0,
    HEAD,
    GET,
    POST,
    PUT,
    DELETE,
    TRACE,
    CONNECT
};

struct RequestOption {
    PROTOCOL   protocol;
    string     host;      // 域名或IP
    int        port;
    string     localaddr; // 指定本地IP
    METHOD     method;
    string     path;
    list<string> headers;
    RequestOption()
        : protocol(HTTP)
        , host("localhost")
        , port(80)
        , method(GET)
        , path("/"){}
};

/** 服务器收到请求回调 */
typedef void(*server_on_request_cb)(Server *server, IncomingMessage *request, ServerResponse *response);
/** 客户端请求收到应答回调 */
typedef void(*client_request_response_cb)(ClientRequest *request, IncomingMessage* response);
/** HTTP客户端请求 */
class ClientRequest
{
public:
    virtual ~ClientRequest(){};
    bool keepAlive; //是否使用长连接
    bool chunked;   //Transfer-Encoding: chunked
    bool finished;  //是否完成
    typedef void(*ReqCb)(ClientRequest *request);
    typedef void(*ResCB)(ClientRequest *request, IncomingMessage* response);
    /** 客户端收到connec方法的应答时回调 */
    ResCB OnConnect;
    /** 客户端收到1xx应答(101除外)时回调 */
    ResCB OnInformation;
    /** 客户端收到101 upgrade 时回调 */
    ResCB OnUpgrade;
    /** 客户端收到应答时回调，如果是其他指定回调，则不会再次进入这里 */
    ResCB OnResponse;
    /**
     * 
     */
    virtual void End(char* data = NULL, int len = 0, ReqCb cb = NULL) = 0;
    /**
     * 用来发送一块数据
     */
    virtual bool Write(char* data, int len, ReqCb cb = NULL) = 0;
protected:
    ClientRequest(){};
};
/** 接收到的数据 */
class IncomingMessage
{

};
/** 应答 */
class ServerResponse
{

};
/** http服务 */
class Server
{

    /** 服务端收到connec方法的请求时回调 */
    typedef void(*connect_cb)(ClientRequest *request, char* head);
};

static const char* METHODS(int m);
static const char* STATUS_CODES(int c);
static Server* createServer(server_on_request_cb cb);
static ClientRequest* request(RequestOption &option, client_request_response_cb cb);

};

}

