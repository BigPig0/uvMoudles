#include "uvnethttp.h"
#include "Log.h"
#include <set>
#include <sstream>

namespace uvNetPlus {
namespace Http {
    //////////////////////////////////////////////////////////////////////////

    static const char *_methods[] = {
        "OPTIONS",
        "HEAD",
        "GET",
        "POST",
        "PUT",
        "DELETE",
        "TRACE",
        "CONNECT"
    };

    static const char *_versions[] = {
        "HTTP/1.0",
        "HTTP/1.1"
    };

    static const char* METHODS(int m){
        return _methods[m];
    }

    static const char* VERSIONS(int v) {
        return _versions[v];
    }

    static const char* STATUS_CODES(int c){
        switch (c)
        {
        case 200:
            return "OK";
        default:
            break;
        }
        return "Unknow status";
    }


    //////////////////////////////////////////////////////////////////////////
    /** 基础发送消息 */

    SendingMessage::SendingMessage()
        : m_bHeadersSent(false)
        , m_bFinished(false)
        , m_pSocket(NULL){}

    SendingMessage::~SendingMessage(){}

    void SendingMessage::WriteHead(std::string headers) {
        m_strHeaders = headers;
    }

    std::vector<std::string> SendingMessage::GetHeader(std::string name) {
        std::vector<std::string> ret;
        auto first = m_Headers.lower_bound(name);
        auto last = m_Headers.upper_bound(name);
        for(;first != last; ++first){
            ret.push_back(first->second);
        }
        return ret;
    }

    std::vector<std::string> SendingMessage::GetHeaderNames() {
        std::set<std::string> tmp;
        std::vector<std::string> ret;
        for(auto &it : m_Headers){
            tmp.insert(it.first);
        }
        for(auto &it : tmp){
            ret.push_back(it);
        }
        return ret;
    }

    bool SendingMessage::HasHeader(std::string name) {
        return m_Headers.count(name) > 0;
    }

    void SendingMessage::RemoveHeader(std::string name) {
        auto pos = m_Headers.equal_range(name);
        for(auto it=pos.first; it!=pos.second; ++it)
            m_Headers.erase(it);
    }

    void SendingMessage::SetHeader(std::string name, std::string value) {
        RemoveHeader(name);
        m_Headers.insert(make_pair(name, value));
    }

    void SendingMessage::SetHeader(std::string name, char **values) {
        RemoveHeader(name);
        
        for(int i = 0; values[i]; i++) {
            m_Headers.insert(make_pair(name, values[i]));
        }
    }

    bool SendingMessage::Finished() {
        return m_bFinished;
    }

    std::string SendingMessage::getImHeaderString() {
        std::stringstream ss;
        for(auto &h:m_Headers) {
            ss << h.first << ": " << h.second << "\r\n";
        }
        return ss.str();
    }

    //////////////////////////////////////////////////////////////////////////
    /** 客户端发送请求 */

    struct requestData {
        typedef void(*ReqCb)(ClientRequest *request);
        char* data;
        int len;
        ReqCb cb;
    };

    ClientRequest::ClientRequest(CTcpConnPool *pool)
        : m_pTcpPool(pool)
        , protocol(HTTP)
        , method(GET)
        , path("/")
        , version(HTTP1_1)
        , host("localhost")
        , port(80)
        , localport(0)
        , keepAlive(true)
        , chunked(false)
        , OnConnect(NULL)
        , OnInformation(NULL)
        , OnUpgrade(NULL)
        , OnResponse(NULL)
    {}

    ClientRequest::~ClientRequest(){}

    bool ClientRequest::Write(char* chunk, int len, ReqCb cb){
        requestData reqData = {chunk, len, cb};
        if(!m_bHeadersSent) {
            stringstream ss;
            ss << GetHeadersString() << "\r\n";
            if(chunked && method == HTTP1_1) {
                ss << std::hex << len << "\r\n";
                ss.write(chunk, len);
                ss << "\r\n";
            } else {
                ss.write(chunk, len);
            }
            string buff = ss.str();
            m_pTcpReq = m_pTcpPool->Request(host, port, "");
            m_pTcpReq->Request(buff.c_str(), buff.size());
            m_bHeadersSent = true;
        } else {
            if(chunked && version == HTTP1_1) {
                stringstream ss;
                ss << std::hex << len << "\r\n";
                ss.write(chunk, len);
                ss << "\r\n";
                string buff = ss.str();
                m_pTcpReq->Request(buff.c_str(), buff.size());
            } else {
                m_pTcpReq->Request(chunk, len);
            }
        }
        return true;
    }

    bool ClientRequest::End() {
        m_bFinished = true;
        return true;
    }

    void ClientRequest::End(char* data, int len, ReqCb cb){
        Write(data, len, cb);
        End();
    }

    std::string ClientRequest::GetAgentName() {
        stringstream ss;
        ss << host << ":" << port;
        if(!localaddr.empty())
            ss << ":" << localaddr;
        if(!localport)
            ss << ":" << localport;
        return ss.str();
    }

    std::string ClientRequest::GetHeadersString() {
        if(!m_strHeaders.empty())
            return m_strHeaders;

        stringstream ss;
        ss << METHODS(method) << " " << path << " " << VERSIONS(version) << "\r\n"
            << "Host: " << host << "\r\n";
        if(keepAlive && method == HTTP1_1)
            ss << "Connection: keep-alive\r\n";
        else
            ss << "Connection: close\r\n";
        if(chunked)
            ss << "Transfer-Encoding: chunked\r\n";
        for(auto &it:m_Headers) {
            ss << it.first << ": " << it.second << "\r\\n";
        }
        return ss.str();
    }

    //////////////////////////////////////////////////////////////////////////
    /** 服务端接收到的请求或客户端接收到的应答 */

    IncomingMessage::IncomingMessage(){}

    IncomingMessage::~IncomingMessage(){}


    //////////////////////////////////////////////////////////////////////////
    /** 服务端生成应答数据并发送 */

    struct responseData {
        typedef void(*ResCb)(ServerResponse *request);
        char* data;
        int len;
        ResCb cb;
    };


    ServerResponse::ServerResponse()
        : sendDate(true)
        , statusCode(200)
        , version(HTTP1_1)
        , keepAlive(true)
        , chunked(false)
        , OnClose(NULL)
        , OnFinish(NULL)
    {}

    ServerResponse::~ServerResponse(){}

    void ServerResponse::AddTrailers(std::string key, std::string value) {
        m_Trailers.insert(make_pair(key, value));
    }

    void ServerResponse::WriteContinue() {

    }

    void ServerResponse::WriteProcessing() {

    }

    void ServerResponse::WriteHead(int statusCode, std::string statusMessage, std::string headers) {
        stringstream ss;
        ss << VERSIONS(version) << " " << statusCode << " ";
        if(!statusMessage.empty())
            ss << statusMessage;
        else
            ss << STATUS_CODES(statusCode);
        ss << "\r\n"
            << headers;
        SendingMessage::WriteHead(ss.str());
    }

    void ServerResponse::Write(char* chunk, int len, ResCb cb) {
        responseData reqData = {chunk, len, cb};
        if(!m_bHeadersSent) {
            stringstream ss;
            ss << GetHeadersString() << "\r\n";
            if(chunked && version == HTTP1_1) {
                ss << std::hex << len << "\r\n";
                ss.write(chunk, len);
                ss << "\r\n";
            } else {
                ss.write(chunk, len);
            }
            string buff = ss.str();
            m_pSocket->Send(buff.c_str(), buff.size());
            m_bHeadersSent = true;
        } else {
            if(chunked && version == HTTP1_1) {
                stringstream ss;
                ss << std::hex << len << "\r\n";
                ss.write(chunk, len);
                ss << "\r\n";
                string buff = ss.str();
                m_pSocket->Send(buff.c_str(), buff.size());
            } else {
                m_pSocket->Send(chunk, len);
            }
        }
    }

    void ServerResponse::End() {
        m_bFinished = true;
    }

    void ServerResponse::End(char* data, int len, ResCb cb) {
        Write(data, len, cb);
        End();
    }

    std::string ServerResponse::GetHeadersString() {
        if(!m_strHeaders.empty())
            return m_strHeaders;

        stringstream ss;
        ss << VERSIONS(version) << " " << statusCode << " ";
        if(!statusMessage.empty())
            ss << statusMessage;
        else
            ss << STATUS_CODES(statusCode);
        ss << "\r\n";
        for(auto &it:m_Headers) {
            ss << it.first << ": " << it.second << "\r\\n";
        }
        return ss.str();
    }


    //////////////////////////////////////////////////////////////////////////
    /** http服务 */

    void Server::OnListen(CTcpServer* svr, std::string err) {
        Server *http = (Server*)svr->userData;
        if(err.empty()) {
            //开启监听成功
            Log::debug("Http server listen %d begining...", http->m_nPort);
        } else {
            //监听失败
            Log::error("Http server listen failed: %s", err.c_str());
        }
    }

    void Server::OnTcpConnection(CTcpServer* svr, std::string err, CTcpClient* client) {
        
    }

    Server::Server(CUVNetPlus* net)
        : m_nPort(0)
        , OnCheckContinue(NULL)
        , OnCheckExpectation(NULL)
        , OnUpgrade(NULL)
        , OnRequest(NULL)
    {
        m_pTcpSvr = CTcpServer::Create(net, OnTcpConnection, this);
        m_pTcpSvr->OnListen = OnListen;
    }

    Server::~Server(){}

    bool Server::Listen(std::string strIP, uint32_t nPort){
        return m_pTcpSvr->Listen(strIP, nPort);
    }

    void Server::Close() {

    }

    bool Server::Listening() {
        return m_pTcpSvr->Listening();
    }

    //////////////////////////////////////////////////////////////////////////




}
}