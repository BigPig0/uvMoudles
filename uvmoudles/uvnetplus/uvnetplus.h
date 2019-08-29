#pragma once
#include "uvnetpuclic.h"
#include <string>
#include <stdint.h>

namespace uvNetPlus {

class CNet
{
public:
    static CNet* Create();
    virtual ~CNet(){};
};

class CTcpClient
{
public:
    typedef void (*fnOnTcpEvent)(CTcpClient* skt);
    typedef void (*fnOnTcpRecv)(CTcpClient* skt, char *data, int len);
    typedef void (*fnOnTcpError)(CTcpClient* skt, std::string error);

    static CTcpClient* Create(CNet* net, fnOnTcpEvent onReady, void *usr=nullptr);
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


}

