#pragma once
#include "uvnetpuclic.h"
#include "uvnettcp.h"
#include <string>
#include <stdint.h>
#include <list>
#include <unordered_map>

namespace uvNetPlus {
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

class ClientRequest;
class IncomingMessage;
class ServerResponse;
class Server;

/** http协议使用的socket的发送操作 */
class SendingMessage
{
public:
    SendingMessage();
    virtual ~SendingMessage();

    /**
     * 显示填写http头，调用后隐式http头的接口就无效了
     * @param headers http头域完整字符串，包含每一行结尾的"\r\n"
     */
    void writeHead(std::string headers);

    /**
     * 获取已经设定的隐式头
     */
    std::vector<std::string> getHeader(std::string name);

    /**
     * 获取所有设定的隐式头的key
     */
    std::vector<std::string> getHeaderNames();

    /**
     * 隐式头是否已经包含一个名称
     */
    bool hasHeader(std::string name);

    /**
     * 移除一个隐式头
     */
    void removeHeader(std::string name);

    /**
     * 重新设置一个头的值，或者新增一个隐式头
     * param name field name
     * param value 单个field value
     * param values 以NULL结尾的字符串数组，多个field value
     */
    void setHeader(std::string name, std::string value);
    void setHeader(std::string name, char **values);

    /**
     * 查看是否完成
     */
    bool finished();

protected:
    /** 由隐式头组成字符串 */
    std::string getHeaderString();

protected:
    std::string         m_strHeaders;  // 显式的头
    hash_list           m_Headers;     // 隐式头的内容
    bool                m_bHeadersSent; // header是否已经发送
    bool                m_bFinished;   // 接收应答是否完成
    CTcpClient*         m_pSocket;     // tcp连接
};


/** HTTP客户端请求,用于客户端组织数据并发送 */
class ClientRequest : public SendingMessage
{
    typedef void(*ReqCb)(ClientRequest *request);
    typedef void(*ResCB)(ClientRequest *request, IncomingMessage* response);
public:
    ClientRequest(CTcpConnPool *pool);
    ~ClientRequest();

    PROTOCOL            protocol;  // 协议,http或https
    METHOD              method;    // 方法
    std::string         path;      // 请求路径
    VERSION             version;   // http版本号 1.0或1.1
    std::string         host;      // 域名或IP
    int                 port;      // 端口
    std::string         localaddr; // 指定本地IP，默认为空
    int                 localport; // 指定本地端口， 默认为0
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

    /**
     * 用来发送一块数据，如果chunked=true，发送一个chunk的数据
     * 如果chunked=false，使用这个方法多次发送数据，必须自己在设置头里设置length
     * @param data 需要发送的数据
     * @param len 发送的数据长度
     * @param cb 数据写入缓存后调用
     */
    bool Write(char* data, int len, ReqCb cb = NULL);

    /**
     * 完成一个发送请求，如果有未发送的部分则将其发送，如果chunked=true，额外发送结束段'0\r\n\r\n'
     * 如果chunked=false,协议头没有发送，则自动添加length
     */
    bool End();

    /**
     * 相当于Write(data, len, cb);end();
     */
    void End(char* data, int len, ReqCb cb = NULL);


private:
    string GetAgentName();

    CTcpConnPool        *m_pTcpPool;
};

/** 接收到的数据，服务端接收到的请求或客户端接收到的应答 */
class IncomingMessage
{
public:
    IncomingMessage();
    ~IncomingMessage();

    bool aborted;   //请求终止时设置为true
    bool complete;  //http消息接收完整时设置为true

    METHOD      method;     // 请求方法
    std::string url;        // 请求路径

    int         statusCode;     //应答状态码
    std::string statusMessage;  //应答状态消息

    std::string httpVersion;    //http版本号 1.1或1.0
    std::string rawHeaders;     //完整的头部字符串
    std::string rawTrailers;    //完整的尾部字符串
    hash_list   headers;        //解析好的http头部键值对
    hash_list   trailers;       //解析好的http尾部键值对
};

/** 服务端生成应答数据并发送 */
class ServerResponse
{
    typedef void(*ResCb)(ServerResponse *response);
public:
    ServerResponse();
    ~ServerResponse();

    /** 发送完成前,socket中断了会回调该方法 */
    ResCb OnClose;
    /** 应答发送完成时回调，所有数据都已经发送 */
    ResCb OnFinish;

    /**
     * 添加一个尾部数据
     * @param key 尾部数据的field name，这个值已经在header中的Trailer里定义了
     * @param value 尾部数据的field value
     */
    void addTrailers(std::string key, std::string value);

    /**
     * 如果调用了此方法，但没有调用writeHead()，则使用隐式头并立即发送头
     */
    void write(char* chunk, int len);

    /**
     * Sends a HTTP/1.1 100 Continue message。包括write和end的功能
     */
    void writeContinue();

    /**
     * Sends a HTTP/1.1 102 Processing message to the client
     */
    void writeProcessing();

    /**
     * 显示填写http头，调用后隐式http头的接口就无效了
     * @param statusCode 响应状态码
     * @param statusMessage 自定义状态消息，可以为空，则使用标准消息
     * @param headers http头域完整字符串，每行都要包含"\r\n"
     */
    void writeHead(int statusCode, std::string statusMessage, std::string headers);

    /**
     * 表明应答的所有数据都已经发送。每个实例都需要调用一次end。执行后会触发OnFinish
     */
    void end();

    /**
     * 相当于调用write(data, len) ; end()
     */
    void end(char* data, int len);

    bool sendDate;    // 默认true，在发送头时自动添加Date头(已存在则不会添加)
    int  statusCode;  // 状态码
    std::string statusMessage; //自定义的状态消息，如果为空，发送时会取标准消息
};

/** http服务 */
class Server
{
    typedef void(*ReqCb)(Server *server, IncomingMessage *request, ServerResponse *response);
public:
    Server(CUVNetPlus* net);
    ~Server();

    /** 服务器启动监听 */
    void listen(int port);
    /** 服务器关闭 */
    void close();

    bool listening; //服务器是否在监听连接


    /** 接受到一个包含'Expect: 100-continue'的请求时调用，如果没有指定，自动发送'100 Continue' */
    ReqCb OnCheckContinue;
    /** 接收到一个包含Expect头，但不是100的请求时调用，如果没有指定，自动发送'417 Expectation Failed' */
    ReqCb OnCheckExpectation;
    /** 收到upgrade请求时调用 */
    ReqCb OnUpgrade;
    /** 接收到一个请求，如果是其他指定的回调，就不会进入这里 */
    ReqCb OnRequest;
};

};

}