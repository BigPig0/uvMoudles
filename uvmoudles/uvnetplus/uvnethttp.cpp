#include "uvnethttp.h"
#include "util_def.h"
#include "Log.h"
#include "bnf.h"
#include "utilc_string.h"
#include "StringHandle.h"
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

    static const char* METHODS(METHOD m){
        return _methods[m];
    }

    static const METHOD METHODS(const char* m) {
        for(int i=0; i<8; ++i) {
            if(!strcasecmp(_methods[i], m))
                return (METHOD)i;
        }
        return (METHOD)-1;
    }

    static const char* VERSIONS(VERSION v) {
        return _versions[v];
    }

    static const VERSION VERSIONS(const char* v) {
        for(int i=0; i<2; ++i) {
            if(!strcasecmp(_versions[i], v))
                return (VERSION)i;
        }
        return (VERSION)-1;
    }

    static const char* STATUS_CODES(int c){
        switch (c) {
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 203: return "Non-Authoritative information";
        case 204: return "No Content";
        case 205: return "Reset Content";
        case 206: return "Partial Content";
        case 300: return "Multiple Choices";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 305: return "Use Proxy";
        case 307: return "Temporary Redirect";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 402: return "Payment Required";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 407: return "Proxy Authentication Required";
        case 408: return "Request Timeout";
        case 409: return "Confilict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Request Entity Too Large	";
        case 414: return "Request-URI Too Long	";
        case 415: return "Unsupported Media Type";
        case 416: return "Requested Range Not Satisfiable";
        case 417: return "Expectation Failed";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        default: break;
        }
        return "Unknow status";
    }


    //////////////////////////////////////////////////////////////////////////
    /** 基础发送消息 */

    SendingMessage::SendingMessage()
        : m_bHeadersSent(false)
        , m_bFinished(false)
        , m_nContentLen(0)
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
        if(!strcasecmp(name.c_str(), "Content-Length")) {
            m_nContentLen = stoi(value);
            return;
        }
        RemoveHeader(name);
        m_Headers.insert(make_pair(name, value));
    }

    void SendingMessage::SetHeader(std::string name, char **values) {
        RemoveHeader(name);
        
        for(int i = 0; values[i]; i++) {
            m_Headers.insert(make_pair(name, values[i]));
        }
    }

    void SendingMessage::SetContentLen(uint32_t len) {
        m_nContentLen = len;
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
    CHttpRequest::CHttpRequest()
        : protocol(HTTP)
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

    CHttpRequest::~CHttpRequest(){}

    CHttpRequest* CHttpRequest::Create(CTcpConnPool *pool) {
        ClientRequest *req = new ClientRequest(pool);
        return req;
    }

    struct requestData {
        typedef void(*ReqCb)(CHttpRequest *request);
        char* data;
        int len;
        ReqCb cb;
    };

    ClientRequest::ClientRequest(CTcpConnPool *pool)
        : m_pTcpPool(pool)
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
            m_pTcpReq->Request(buff.c_str(), (int)buff.size());
            m_bHeadersSent = true;
        } else {
            if(chunked && version == HTTP1_1) {
                stringstream ss;
                ss << std::hex << len << "\r\n";
                ss.write(chunk, len);
                ss << "\r\n";
                string buff = ss.str();
                m_pTcpReq->Request(buff.c_str(), (int)buff.size());
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

    void ClientRequest::WriteHead(std::string headers) {
        SendingMessage::WriteHead(headers);
    }

    std::vector<std::string> ClientRequest::GetHeader(std::string name) {
        return SendingMessage::GetHeader(name);
    }

    std::vector<std::string> ClientRequest::GetHeaderNames() {
        return SendingMessage::GetHeaderNames();
    }

    bool ClientRequest::HasHeader(std::string name) {
        return SendingMessage::HasHeader(name);
    }

    void ClientRequest::RemoveHeader(std::string name) {
        SendingMessage::RemoveHeader(name);
    }

    void ClientRequest::SetHeader(std::string name, std::string value) {
        SendingMessage::SetHeader(name, value);
    }

    void ClientRequest::SetHeader(std::string name, char **values) {
        SendingMessage::SetHeader(name, values);
    }

    void ClientRequest::SetContentLen(uint32_t len){
        SendingMessage::SetContentLen(len);
    }

    bool ClientRequest::Finished() {
        return SendingMessage::Finished();
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
    /** 服务端生成应答数据并发送 */

    CHttpResponse::CHttpResponse()
        : sendDate(true)
        , statusCode(200)
        , version(HTTP1_1)
        , keepAlive(true)
        , chunked(false)
        , OnClose(NULL)
        , OnFinish(NULL)
    {}

    CHttpResponse::~CHttpResponse() {}

    struct responseData {
        typedef void(*ResCb)(CHttpResponse *request);
        char* data;
        int len;
        ResCb cb;
    };


    ServerResponse::ServerResponse()
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
            Log::debug(buff.c_str());
            m_pSocket->Send(buff.c_str(), (uint32_t)buff.size());
            m_bHeadersSent = true;
        } else {
            if(chunked && version == HTTP1_1) {
                stringstream ss;
                ss << std::hex << len << "\r\n";
                ss.write(chunk, len);
                ss << "\r\n";
                string buff = ss.str();
                m_pSocket->Send(buff.c_str(), (uint32_t)buff.size());
            } else {
                m_pSocket->Send(chunk, len);
            }
        }
    }

    void ServerResponse::End() {
        m_bFinished = true;
    }

    void ServerResponse::End(char* data, int len, ResCb cb) {
        if(!chunked && !m_nContentLen) {
            m_nContentLen = len;
        }
        Write(data, len, cb);
        End();
    }

    void ServerResponse::WriteHead(std::string headers) {
        SendingMessage::WriteHead(headers);
    }

    std::vector<std::string> ServerResponse::GetHeader(std::string name) {
        return SendingMessage::GetHeader(name);
    }

    std::vector<std::string> ServerResponse::GetHeaderNames() {
        return SendingMessage::GetHeaderNames();
    }

    bool ServerResponse::HasHeader(std::string name) {
        return SendingMessage::HasHeader(name);
    }

    void ServerResponse::RemoveHeader(std::string name) {
        SendingMessage::RemoveHeader(name);
    }

    void ServerResponse::SetHeader(std::string name, std::string value) {
        SendingMessage::SetHeader(name, value);
    }

    void ServerResponse::SetHeader(std::string name, char **values) {
        SendingMessage::SetHeader(name, values);
    }

    void ServerResponse::SetContentLen(uint32_t len){
        SendingMessage::SetContentLen(len);
    }

    bool ServerResponse::Finished() {
        return SendingMessage::Finished();
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
        ss << "\r\nContent-Length: "
            << m_nContentLen << "\r\n";
        for(auto &it:m_Headers) {
            ss << it.first << ": " << it.second << "\r\n";
        }
        return ss.str();
    }

    //////////////////////////////////////////////////////////////////////////
    /** 服务端接收到的请求或客户端接收到的应答 */

    CIncomingMessage::CIncomingMessage()
        : aborted(false)
        , complete(false)
        , keepAlive(true)
        , chunked(false)
        , contentLen(0)
    {}

    CIncomingMessage::~CIncomingMessage(){}

    IncomingMessage::IncomingMessage(){}

    IncomingMessage::~IncomingMessage(){}


    //////////////////////////////////////////////////////////////////////////
    /** http服务 */

    CHttpServer::CHttpServer()
        : OnCheckContinue(NULL)
        , OnCheckExpectation(NULL)
        , OnUpgrade(NULL)
        , OnRequest(NULL)
    {}

    CHttpServer::~CHttpServer(){}

    CHttpServer* CHttpServer::Create(CNet* net){
        Server *svr = new Server(net);
        return svr;
    }

    CSvrConn::CSvrConn()
        : inc(nullptr)
        , res(nullptr)
        , parseHeader(false)
    {}

    bool CSvrConn::ParseHeader() {
        size_t pos1 = buff.find("\r\n");        //第一行的结尾
        size_t pos2 = buff.find("\r\n\r\n");    //头的结尾位置
        string reqline = buff.substr(0, pos1);  //第一行的内容
        vector<string> reqlines = StringHandle::StringSplit(reqline, ' ');
        if(reqlines.size() != 3)
            return false;

        inc = new IncomingMessage();
        res = new ServerResponse();
        res->m_pSocket = client;

        inc->method = METHODS(reqlines[0].c_str());
        inc->url = reqlines[1];
        inc->version = VERSIONS(reqlines[2].c_str());
        inc->rawHeaders = buff.substr(pos1+2, pos2-pos1);
        buff = buff.substr(pos2+4, buff.size()-pos2-4);

        if(inc->version == HTTP1_0)
            inc->keepAlive = false;
        else
            inc->keepAlive = true;

        vector<string> headers = StringHandle::StringSplit(inc->rawHeaders, "\r\n");
        for(auto &hh : headers) {
            string name, value;
            bool b = false;
            for(auto &c:hh){
                if(!b) {
                    if(c == ':'){
                        b = true;
                    } else {
                        name.push_back(c);
                    }
                } else {
                    if(!value.empty() || c != ' ')
                        value.push_back(c);
                }
            }
            // 检查关键头
            if(!strcasecmp(name.c_str(), "Connection")) {
                if(!strcasecmp(value.c_str(), "Keep-Alive"))
                    inc->keepAlive = true;
                else if(!strcasecmp(value.c_str(), "Close"))
                    inc->keepAlive = false;
            } else if(!strcasecmp(name.c_str(), "Content-Length")) {
                inc->contentLen = stoi(value);
            } else if(!strcasecmp(name.c_str(), "Transfer-Encoding")) {
                if(inc->version != HTTP1_0 && !strcasecmp(value.c_str(), "chunked"))
                    inc->chunked = true;
            }
            //保存头
            inc->headers.insert(make_pair(name, value));
        }
        return true;
    }

    bool CSvrConn::ParseContent() {
        if(inc->chunked) {
            size_t pos = buff.find("\r\n");
            int len = htoi(buff.substr(0, pos).c_str());
            if(buff.size() - pos - 4 >= len) {
                // 接收完整块
                if(len==0)
                    inc->complete = true;
                inc->contentLen = len;
                inc->content = buff.substr(pos+2, len);
                buff = buff.substr(pos+len+4,buff.size()-pos-len-4);
                return true;
            }
            return false;
        }

        //chunked false时的情况
        if(inc->contentLen == 0) {
            // 没有设置长度，永不停止接收数据。这不是标准协议，自定义的处理
            inc->content = buff;
            buff.clear();
            return true;
        }

        if(buff.size() >= inc->contentLen) {
            inc->content = buff.substr(0, inc->contentLen);
            buff = buff.substr(inc->contentLen, buff.size()-inc->contentLen);
            inc->complete = true;
            return true;
        }

        return false;
    }

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

    void Server::OnSvrCltRecv(CTcpClient* skt, char *data, int len){
        CSvrConn *c = (CSvrConn*)skt->userData;
        c->buff.append(data, len);
        // http头解析
        if(!c->parseHeader && c->buff.find("\r\n\r\n") != std::string::npos) {
            if(!c->ParseHeader()) {
                Log::error("error request");
                return;
            }
        }
        //http内容解析
        if(c->inc->method == POST){
            while(c->ParseContent()) {
                // 接收到的数据可以上抛回调
                if(c->http->OnRequest && !c->inc->content.empty())
                    c->http->OnRequest(c->http, c->inc, c->res);
                //是否接收完成
                if(c->inc->complete) {
                    //一个请求解析完成，重置
                    SAFE_DELETE(c->inc);
                    SAFE_DELETE(c->res);
                    c->parseHeader = false;
                    if(!c->buff.empty()){
                        // 已经接收到下一个请求
                        string tmp = c->buff;
                        c->buff.clear();
                        OnSvrCltRecv(skt, (char*)tmp.c_str(), (int)tmp.size());
                    }
                }
            }
        } else {
            c->inc->complete = true;
            // 接收到的数据可以上抛回调
            if(c->http->OnRequest)
                c->http->OnRequest(c->http, c->inc, c->res);
            //一个请求解析完成，重置
            SAFE_DELETE(c->inc);
            SAFE_DELETE(c->res);
            c->parseHeader = false;
            if(!c->buff.empty()){
                // 已经接收到下一个请求
                string tmp = c->buff;
                c->buff.clear();
                OnSvrCltRecv(skt, (char*)tmp.c_str(), (int)tmp.size());
            }
        }
    }

    void Server::OnSvrCltDrain(CTcpClient* skt){
        CSvrConn *c = (CSvrConn*)skt->userData;
        Log::debug("server client drain");
    }

    void Server::OnSvrCltClose(CTcpClient* skt){
        CSvrConn *c = (CSvrConn*)skt->userData;
        Log::debug("server client close");
        delete c;
    }

    void Server::OnSvrCltEnd(CTcpClient* skt){
        CSvrConn *c = (CSvrConn*)skt->userData;
        Log::debug("server client end");
    }

    void Server::OnSvrCltError(CTcpClient* skt, string err){
        CSvrConn *c = (CSvrConn*)skt->userData;
        Log::error("server client error:%s ", err.c_str());
    }

    void Server::OnTcpConnection(CTcpServer* svr, std::string err, CTcpClient* client) {
        Log::debug("Http server accept new connection");
        Server *http = (Server*)svr->userData;
        static int sid = 1;
        CSvrConn *c = new CSvrConn();
        c->http   = http;
        c->server = svr;
        c->client = client;
        client->userData = c;
        client->OnRecv = OnSvrCltRecv;
        client->OnDrain = OnSvrCltDrain;
        client->OnCLose = OnSvrCltClose;
        client->OnEnd = OnSvrCltEnd;
        client->OnError = OnSvrCltError;
    }

    Server::Server(CNet* net)
        : m_nPort(0)
    {
        m_pTcpSvr = CTcpServer::Create(net, OnTcpConnection, this);
        m_pTcpSvr->OnListen = OnListen;
    }

    Server::~Server(){}

    bool Server::Listen(std::string strIP, uint32_t nPort){
        m_nPort = nPort;
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