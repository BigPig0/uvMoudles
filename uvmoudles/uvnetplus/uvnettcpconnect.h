/*!
 * \file uvnettcpconnect.h
 * \date 2019/09/05 16:11
 *
 * \author wlla
 * Contact: user@company.com
 *
 * \brief 
 *
 * TODO: 实现CTcpConnect接口。
 * 功能是向指定地址发送、接收数据。内部维护连接池，外部只可见请求和应答。
 * 可以设置只发送不应答 发送应答。有应答时外部判断应答完成
 *
 * \note
*/

#pragma once
#include "uvnetplus.h"
#include "uvnetprivate.h"
#include "uvnettcp.h"
#include <list>
using namespace std;

namespace uvNetPlus {

class CUNTcpConnPool;
class CUNTcpRequest;

enum ConnState { 
    ConnState_Idle = 0, //空闲，在空闲列表中
    ConnState_snd,      //发送请求
    ConnState_sndok,    //发送完成
    ConnState_rcv,      //收到应答
};

class TcpConnect {
public:
    TcpConnect(CUNTcpConnPool *p, CUNTcpRequest *request);
    ~TcpConnect();

    CUNTcpConnPool *pool;     //连接所在的连接池
    CUNTcpClient   *client;   //客户端连接实例
    CUNTcpRequest  *req;      //当前执行的请求
    string          ip;       //服务器ip
    uint32_t        port;     //服务器端口
    time_t          lastTime; //最后通讯时间
    ConnState       state;    //连接状态
};

class CUNTcpRequest : public TcpRequest {
public:
    CUNTcpRequest();
    ~CUNTcpRequest();

    virtual void Request(const char* buff, int length);
    virtual void Finish();

    CUNTcpConnPool *pool;    //连接所在的连接池
    TcpConnect     *conn;    //请求所使用的连接
};

class CUNTcpConnPool : public CTcpConnPool
{
public:
    CUNTcpConnPool(CUVNetPlus* net);
    ~CUNTcpConnPool();

    void syncInit();
    void syncRequest();
    void syncClose();

    virtual void Delete();

    virtual TcpRequest* Request(string ip, uint32_t port,  const char* data, int len, void *usr=nullptr, bool copy=true, bool recv=true);

    virtual void MaxConns(uint32_t num){m_nMaxConns = num;}

    virtual void MaxIdle(uint32_t num){m_nMaxIdle = num;}

    fnOnConnectRequest      m_funOnRequest;
    fnOnConnectResponse     m_funOnResponse;

public:
    CUVNetPlus         *m_pNet;         //事件线程句柄
    uint32_t            m_nMaxConns;    //最大连接数 默认512(busy+idle)
    uint32_t            m_nMaxIdle;     //最大空闲连接数 默认100
    uint32_t            m_nBusyCount;   //当前使用中的连接数
    uint32_t            m_nIdleCount;   //当前空闲的连接数
    uint32_t            m_nTimeOut;     //空闲连接超时时间 秒

    list<TcpConnect*>   m_listBusyConns;    //正在使用中的连接
    list<TcpConnect*>   m_listIdleConns;    //空闲连接 front时间较近 back时间较久
    list<CUNTcpRequest*> m_listReqs;      //请求列表
    uv_mutex_t          m_ReqMtx;        //请求列表锁

    uv_timer_t         *m_uvTimer;     //定时器用来判断空闲连接是否超时
};
}