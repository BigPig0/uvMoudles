#include "uvnethttp.h"
#include <set>
#include <sstream>

namespace uvNetPlus {
namespace Http {
    //////////////////////////////////////////////////////////////////////////
    /** 基础发送消息 */
    SendingMessage::SendingMessage()
        : m_bHeadersSent(false)
        , m_bFinished(false)
        , m_pSocket(NULL){}

    SendingMessage::~SendingMessage(){}

    void SendingMessage::writeHead(std::string headers) {
        m_strHeaders = headers;
    }

    std::vector<std::string> SendingMessage::getHeader(std::string name) {
        std::vector<std::string> ret;
        auto first = m_Headers.lower_bound(name);
        auto last = m_Headers.upper_bound(name);
        for(;first != last; ++first){
            ret.push_back(first->second);
        }
        return ret;
    }

    std::vector<std::string> SendingMessage::getHeaderNames() {
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

    bool SendingMessage::hasHeader(std::string name) {
        return m_Headers.count(name) > 0;
    }

    void SendingMessage::removeHeader(std::string name) {
        auto pos = m_Headers.equal_range(name);
        for(auto it=pos.first; it!=pos.second; ++it)
            m_Headers.erase(it);
    }

    void SendingMessage::setHeader(std::string name, std::string value) {
        removeHeader(name);
        m_Headers.insert(make_pair(name, value));
    }

    void SendingMessage::setHeader(std::string name, char **values) {
        removeHeader(name);
        
        for(int i = 0; values[i]; i++) {
            m_Headers.insert(make_pair(name, values[i]));
        }
    }

    bool SendingMessage::finished() {
        return m_bFinished;
    }

    std::string SendingMessage::getHeaderString() {
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

    bool ClientRequest::Write(char* data, int len, ReqCb cb){
        requestData reqData = {data, len, cb};
        if(!m_bHeadersSent) {
            string headers;
            if(m_strHeaders.empty()) {

            }
        }
    }

    bool End();

    void ClientRequest::End(char* data, int len, ReqCb cb){}

    string ClientRequest::GetAgentName() {

    }

    //////////////////////////////////////////////////////////////////////////
    /** 服务端接收到的请求或客户端接收到的应答 */

    IncomingMessage::IncomingMessage(){}

    IncomingMessage::~IncomingMessage(){}


    //////////////////////////////////////////////////////////////////////////
    /** 服务端生成应答数据并发送 */

    ServerResponse::ServerResponse(){}

    ServerResponse::~ServerResponse(){}


    //////////////////////////////////////////////////////////////////////////
    /** http服务 */

    Server::Server(){}

    Server::~Server(){}


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

    const char* METHODS(int m){
        return _methods[m];
    }

    const char* STATUS_CODES(int c){
        switch (c)
        {
        case 200:
            return "OK";
        default:
            break;
        }
        return "Unknow status";
    }


}
}