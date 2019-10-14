#pragma once
#include "uvnetpuclic.h"
#include <string>
#include <list>
#include <unordered_map>
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
namespace Http {
typedef std::unordered_multimap<std::string, std::string> hash_list;

enum METHOD {
    OPTIONS = 0,
    HEAD,
    GET,
    POST,
    PUT,
    DEL,
    TRACE,
    CONNECT
};

enum VERSION {
    HTTP1_0 = 0,
    HTTP1_1,
    HTTP2,
    HTTP3
};

class CIncomingMessage {
public:
    bool aborted;   //请求终止时设置为true
    bool complete;  //http消息接收完整时设置为true

    METHOD      method;     // 请求方法
    std::string url;        // 请求路径

    int         statusCode;     //应答状态码
    std::string statusMessage;  //应答状态消息

    VERSION     version;        //http版本号 1.1或1.0
    std::string rawHeaders;     //完整的头部字符串
    std::string rawTrailers;    //完整的尾部字符串
    hash_list   headers;        //解析好的http头部键值对
    hash_list   trailers;       //解析好的http尾部键值对
    bool        keepAlive;      // 是否使用长连接, true时，使用CTcpConnPool管理连接
    bool        chunked;        // Transfer-Encoding: chunked
    uint32_t    contentLen;     // chunked为false时：内容长度；chunked为true时，块长度
    std::string content;        // 一次的接收内容
protected:
    CIncomingMessage();
    virtual ~CIncomingMessage() = 0;
};

class CHttpMsg {
public:
    /**
     * 显示填写http头，调用后隐式http头的接口就无效了
     * @param headers http头域完整字符串，包含每一行结尾的"\r\n"
     */
    virtual void WriteHead(std::string headers) = 0;

    /**
     * 获取已经设定的隐式头
     */
    virtual std::vector<std::string> GetHeader(std::string name) = 0;

    /**
     * 获取所有设定的隐式头的key
     */
    virtual std::vector<std::string> GetHeaderNames() = 0;

    /**
     * 隐式头是否已经包含一个名称
     */
    virtual bool HasHeader(std::string name) = 0;

    /**
     * 移除一个隐式头
     */
    virtual void RemoveHeader(std::string name) = 0;

    /**
     * 重新设置一个头的值，或者新增一个隐式头
     * param name field name
     * param value 单个field value
     * param values 以NULL结尾的字符串数组，多个field value
     */
    virtual void SetHeader(std::string name, std::string value) = 0;
    virtual void SetHeader(std::string name, char **values) = 0;

    /**
     * 查看是否完成
     */
    virtual bool Finished() = 0;
};

class CHttpRequest : public CHttpMsg {
public:
    typedef void(*ReqCb)(CHttpRequest *request);
    typedef void(*ResCB)(CHttpRequest *request, CIncomingMessage* response);

    PROTOCOL            protocol;  // 协议,http或https
    METHOD              method;    // 方法
    std::string         path;      // 请求路径
    VERSION             version;   // http版本号 1.0或1.1
    std::string         host;      // 域名或IP
    int                 port;      // 端口
    std::string         localaddr; // 指定本地IP，默认为空
    int                 localport; // 指定本地端口， 默认为0。只有很特殊的情形需要设置，正常都不需要
    bool                keepAlive; // 是否使用长连接, true时，使用CTcpConnPool管理连接
    bool                chunked;   // Transfer-Encoding: chunked


    /** 客户端收到connect方法的应答时回调 */
    ResCB OnConnect;
    /** 客户端收到1xx应答(101除外)时回调 */
    ResCB OnInformation;
    /** 客户端收到101 upgrade 时回调 */
    ResCB OnUpgrade;
    /** 客户端收到应答时回调，如果是其他指定回调，则不会再次进入这里 */
    ResCB OnResponse;

    /** 创建实例 */
    static CHttpRequest* Create(CTcpConnPool *pool);

    /**
     * 用来发送一块数据，如果chunked=true，发送一个chunk的数据
     * 如果chunked=false，使用这个方法多次发送数据，必须自己在设置头里设置length
     * @param chunk 需要发送的数据
     * @param len 发送的数据长度
     * @param cb 数据写入缓存后调用
     */
    virtual bool Write(char* chunk, int len, ReqCb cb = NULL) = 0;

    /**
     * 完成一个发送请求，如果有未发送的部分则将其发送，如果chunked=true，额外发送结束段'0\r\n\r\n'
     * 如果chunked=false,协议头没有发送，则自动添加length
     */
    virtual bool End() = 0;

    /**
     * 相当于Write(data, len, cb);end();
     */
    virtual void End(char* data, int len, ReqCb cb = NULL) = 0;
protected:
    CHttpRequest();
    virtual ~CHttpRequest() = 0;
};

class CHttpResponse : public CHttpMsg {
public:
    typedef void(*ResCb)(CHttpResponse *response);

    bool                sendDate;      // 默认true，在发送头时自动添加Date头(已存在则不会添加)
    int                 statusCode;    // 状态码
    std::string         statusMessage; //自定义的状态消息，如果为空，发送时会取标准消息
    VERSION             version;       // http版本号 1.0或1.1
    bool                keepAlive; // 是否使用长连接, true时，使用CTcpConnPool管理连接
    bool                chunked;   // Transfer-Encoding: chunked

    /** 发送完成前,socket中断了会回调该方法 */
    ResCb OnClose;
    /** 应答发送完成时回调，所有数据都已经发送 */
    ResCb OnFinish;

    /**
     * 添加一个尾部数据
     * @param key 尾部数据的field name，这个值已经在header中的Trailer里定义了
     * @param value 尾部数据的field value
     */
    virtual void AddTrailers(std::string key, std::string value) = 0;

    /**
     * Sends a HTTP/1.1 100 Continue message。包括write和end的功能
     */
    virtual void WriteContinue() = 0;

    /**
     * Sends a HTTP/1.1 102 Processing message to the client
     */
    virtual void WriteProcessing() = 0;

    /**
     * 显示填写http头，调用后隐式http头的接口就无效了
     * @param statusCode 响应状态码
     * @param statusMessage 自定义状态消息，可以为空，则使用标准消息
     * @param headers http头域完整字符串，每行都要包含"\r\n"
     */
    virtual void WriteHead(int statusCode, std::string statusMessage, std::string headers) = 0;

    /**
     * 如果调用了此方法，但没有调用writeHead()，则使用隐式头并立即发送头
     */
    virtual void Write(char* chunk, int len, ResCb cb = NULL) = 0;

    /**
     * 表明应答的所有数据都已经发送。每个实例都需要调用一次end。执行后会触发OnFinish
     */
    virtual void End() = 0;

    /**
     * 相当于调用write(data, len, cb) ; end()
     */
    virtual void End(char* data, int len, ResCb cb = NULL) = 0;

protected:
    CHttpResponse();
    virtual ~CHttpResponse() = 0;
};

class CHttpServer {
    typedef void(*ReqCb)(CHttpServer *server, CIncomingMessage *request, CHttpResponse *response);
public:
    /** 接受到一个包含'Expect: 100-continue'的请求时调用，如果没有指定，自动发送'100 Continue' */
    ReqCb OnCheckContinue;
    /** 接收到一个包含Expect头，但不是100的请求时调用，如果没有指定，自动发送'417 Expectation Failed' */
    ReqCb OnCheckExpectation;
    /** 收到upgrade请求时调用 */
    ReqCb OnUpgrade;
    /** 接收到一个请求，如果是其他指定的回调，就不会进入这里 */
    ReqCb OnRequest;

    /** 创建一个实例 */
    static CHttpServer* Create(CNet* net);

    /** 服务器启动监听 */
    virtual bool Listen(std::string strIP, uint32_t nPort) = 0;
    /** 服务器关闭 */
    virtual void Close() = 0;
    /** 服务器是否在监听连接 */
    virtual bool Listening() = 0;
protected:
    CHttpServer();
    virtual ~CHttpServer() = 0;
};
}; //namespace Http

}

