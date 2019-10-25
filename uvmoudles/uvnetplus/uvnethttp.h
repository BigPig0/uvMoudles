#pragma once
#include "uvnetpuclic.h"
#include "uvnettcp.h"
#include <string>
#include <stdint.h>

namespace uvNetPlus {
namespace Http {

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
    void WriteHead(std::string headers);

    /**
     * 获取已经设定的隐式头
     */
    std::vector<std::string> GetHeader(std::string name);

    /**
     * 获取所有设定的隐式头的key
     */
    std::vector<std::string> GetHeaderNames();

    /**
     * 隐式头是否已经包含一个名称
     */
    bool HasHeader(std::string name);

    /**
     * 移除一个隐式头
     */
    void RemoveHeader(std::string name);

    /**
     * 重新设置一个头的值，或者新增一个隐式头
     * param name field name
     * param value 单个field value
     * param values 以NULL结尾的字符串数组，多个field value
     */
    void SetHeader(std::string name, std::string value);
    void SetHeader(std::string name, char **values);

    /**
     * 设置内容长度。内容分多次发送，且不使用chunked时使用。
     */
    void SetContentLen(uint32_t len);

    /**
     * 查看是否完成
     */
    bool Finished();

    CTcpClient*         m_pSocket;     // tcp连接
protected:
    /** 由隐式头组成字符串 */
    std::string getImHeaderString();

protected:
    std::string         m_strHeaders;   // 显式的头
    hash_list           m_Headers;      // 隐式头的内容
    bool                m_bHeadersSent; // header是否已经发送
    bool                m_bFinished;    // 接收应答是否完成
    uint32_t            m_nContentLen;  // 设置内容的长度
};


/** HTTP客户端请求,用于客户端组织数据并发送 */
class ClientRequest : public SendingMessage, public CHttpRequest
{
public:
    ClientRequest(CTcpConnPool *pool);
    ~ClientRequest();

    /** 删除实例 */
    virtual void Delete();

    /**
     * 用来发送一块数据，如果chunked=true，发送一个chunk的数据
     * 如果chunked=false，使用这个方法多次发送数据，必须自己在设置头里设置length
     * @param chunk 需要发送的数据
     * @param len 发送的数据长度
     * @param cb 数据写入缓存后调用
     */
    virtual bool Write(const char* chunk, int len, ReqCb cb = NULL);

    /**
     * 完成一个发送请求，如果有未发送的部分则将其发送，如果chunked=true，额外发送结束段'0\r\n\r\n'
     * 如果chunked=false,协议头没有发送，则自动添加length
     */
    virtual bool End();

    /**
     * 相当于Write(data, len, cb);end();
     */
    virtual void End(const char* data, int len, ReqCb cb = NULL);

    virtual void WriteHead(std::string headers);
    virtual std::vector<std::string> GetHeader(std::string name);
    virtual std::vector<std::string> GetHeaderNames();
    virtual bool HasHeader(std::string name);
    virtual void RemoveHeader(std::string name);
    virtual void SetHeader(std::string name, std::string value);
    virtual void SetHeader(std::string name, char **values);
    virtual void SetContentLen(uint32_t len);
    virtual bool Finished();

    /* 收到的数据处理 */
    void Receive(const char *data, int len);

private:
    std::string GetAgentName();
    std::string GetHeadersString();

    /** 解析http头，成功返回true，不是http头返回false */
    bool ParseHeader();
    /** 解析内容，已经接收完整内容或块返回true，否则false */
    bool ParseContent();

    CTcpConnPool        *m_pTcpPool;
    CTcpRequest         *m_pTcpReq;
    IncomingMessage     *inc;
    bool                 parseHeader;   //请求报文中解析出http头。默认false
    std::string          buff;   //接收数据缓存
};

/** 服务端生成应答数据并发送 */
class ServerResponse : public SendingMessage, public CHttpResponse
{
public:
    ServerResponse();
    ~ServerResponse();

    /**
     * 添加一个尾部数据
     * @param key 尾部数据的field name，这个值已经在header中的Trailer里定义了
     * @param value 尾部数据的field value
     */
    virtual void AddTrailers(std::string key, std::string value);

    /**
     * Sends a HTTP/1.1 100 Continue message。包括write和end的功能
     */
    virtual void WriteContinue();

    /**
     * Sends a HTTP/1.1 102 Processing message to the client
     */
    virtual void WriteProcessing();

    /**
     * 显示填写http头，调用后隐式http头的接口就无效了
     * @param statusCode 响应状态码
     * @param statusMessage 自定义状态消息，可以为空，则使用标准消息
     * @param headers http头域完整字符串，每行都要包含"\r\n"
     */
    virtual void WriteHead(int statusCode, std::string statusMessage, std::string headers);

    /**
     * 如果调用了此方法，但没有调用writeHead()，则使用隐式头并立即发送头
     */
    virtual void Write(const char* chunk, int len, ResCb cb = NULL);

    /**
     * 表明应答的所有数据都已经发送。每个实例都需要调用一次end。执行后会触发OnFinish
     */
    virtual void End();

    /**
     * 相当于调用write(data, len, cb) ; end()
     */
    virtual void End(const char* data, int len, ResCb cb = NULL);

    virtual void WriteHead(std::string headers);
    virtual std::vector<std::string> GetHeader(std::string name);
    virtual std::vector<std::string> GetHeaderNames();
    virtual bool HasHeader(std::string name);
    virtual void RemoveHeader(std::string name);
    virtual void SetHeader(std::string name, std::string value);
    virtual void SetHeader(std::string name, char **values);
    virtual void SetContentLen(uint32_t len);
    virtual bool Finished();
private:
    std::string GetHeadersString();

    hash_list   m_Trailers;
};

/** 接收到的数据，服务端接收到的请求或客户端接收到的应答 */
class IncomingMessage : public CIncomingMessage
{
public:
    IncomingMessage();
    ~IncomingMessage();
};

/** http服务端连接 */
class CSvrConn {
public:
    CSvrConn();
    /** 解析http头，成功返回true，不是http头返回false */
    bool ParseHeader();
    /** 解析内容，已经接收完整内容或块返回true，否则false */
    bool ParseContent();

    Server          *http;
    CTcpServer      *server;
    CTcpClient      *client;
    std::string      buff;   //接收数据缓存
    IncomingMessage *inc;    //保存解析到的请求数据
    ServerResponse  *res;    //应答
    bool             parseHeader;   //请求报文中解析出http头。默认false，请求完成后要重置为false。
};

/** http服务 */
class Server : public CHttpServer
{
public:
    Server(CNet* net);
    ~Server();

    /** 服务器启动监听 */
    bool Listen(std::string strIP, uint32_t nPort);
    /** 服务器关闭 */
    void Close();
    /** 服务器是否在监听连接 */
    bool Listening();

private:
    static void OnListen(CTcpServer* svr, std::string err);
    static void OnTcpConnection(CTcpServer* svr, std::string err, CTcpClient* client);
    static void OnSvrCltRecv(CTcpClient* skt, char *data, int len);
    static void OnSvrCltDrain(CTcpClient* skt);
    static void OnSvrCltClose(CTcpClient* skt);
    static void OnSvrCltEnd(CTcpClient* skt);
    static void OnSvrCltError(CTcpClient* skt, string err);

private:
    int           m_nPort;      //服务监听端口
    CTcpServer   *m_pTcpSvr;    //tcp监听服务

    std::unordered_multimap<std::string,CSvrConn*> m_pConns;   //所有连接的客户端请求
};

};

}