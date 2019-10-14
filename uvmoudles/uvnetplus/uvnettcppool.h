/*!
 * \file uvnettcppool.h
 * \date 2019/09/05 16:11
 *
 * \author wlla
 * Contact: user@company.com
 *
 * \brief 
 *
 * TODO: 实现CTcpConnect接口。
 * 功能是向指定地址发送、接收数据。内部维护连接池，外部只可见请求和应答。
 * 可以设置只发送不应答、发送应答。有应答时外部判断应答完成
 *
 * \note
*/

#pragma once
#include "uvnetplus.h"
#include "uvnettcp.h"
#include <list>
#include <unordered_map>
using namespace std;

namespace uvNetPlus {

class TcpAgent;
class CUNTcpConnPool;
class CUNTcpRequest;

enum ConnState {
    ConnState_Init = 0, //新创建的连接
    ConnState_Idle,     //空闲，在空闲列表中
    ConnState_snd,      //发送请求
    ConnState_sndok,    //发送完成
    ConnState_rcv,      //收到应答
};

/**
 * tcp客户端连接类
 */
class TcpConnect {
public:
    TcpConnect(TcpAgent *agt, CUNTcpRequest *request, string hostip);
    ~TcpConnect();

    TcpAgent       *m_pAgent;     //连接所在的agent
    CUNTcpClient   *m_pTcpClient; //客户端连接实例
    CUNTcpRequest  *m_pReq;       //当前执行的请求
    string          m_strIP;      //服务器ip 这个地方已将host解析成ip了
    uint32_t        m_nPort;      //服务器端口
    time_t          m_nLastTime;  //最后通讯时间
    ConnState       m_eState;     //连接状态
};

/**
 * tcp客户端agent类，管理一个同一个地址的连接列表
 * 所有的连接都是同样的host和端口，一个host可以是不同的ip
 */
class TcpAgent {
public:
    TcpAgent(CUNTcpConnPool *p);
    ~TcpAgent();

    void HostInfo(string host);
    void Request(CUNTcpRequest *req);

public:
    CUVNetPlus         *m_pNet;         //事件线程句柄
    uint32_t            m_nMaxConns;    //最大连接数 默认512(busy+idle)
    uint32_t            m_nMaxIdle;     //最大空闲连接数 默认100
    uint32_t            m_nTimeOut;     //空闲连接超时时间 秒 默认20s  0：永不超时

    CUNTcpConnPool     *m_pTcpConnPool;      //连接所在的连接池
    string              m_strHost;           //服务器host或ip
    uint32_t            m_nPort;             //服务器端口
    list<string>        m_listIP;            //解析host得到的ip地址，轮流使用，如果不通会移除

    list<TcpConnect*>    m_listBusyConns;    //正在使用中的连接
    list<TcpConnect*>    m_listIdleConns;    //空闲连接 front时间较近 back时间较久
    list<CUNTcpRequest*> m_listReqs;         //请求列表
};

/**
 * TCP请求类，继承自导出接口
 */
class CUNTcpRequest : public CTcpRequest {
public:
    CUNTcpRequest();
    ~CUNTcpRequest();

    virtual void Request(const char* buff, int length);
    virtual void Finish();

    string getAgentName();

    TcpAgent       *agent;   //请求所在的agent
    CUNTcpConnPool *pool;    //连接所在的连接池
    TcpConnect     *conn;    //请求所使用的连接

    list<uv_buf_t>  buffs;   //TcpClient创建之前要求发送的数据进行缓存
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

    virtual CTcpRequest* Request(string host, uint32_t port, string localaddr, void *usr=nullptr, bool copy=true, bool recv=true);

public:
    CUVNetPlus         *m_pNet;         //事件线程句柄
    uint32_t            m_nBusyCount;   //当前使用中的连接数
    uint32_t            m_nIdleCount;   //当前空闲的连接数

    list<CUNTcpRequest*> m_listReqs;      //请求列表,暂存外部的请求
    uv_mutex_t           m_ReqMtx;        //请求列表锁

    uv_timer_t         *m_uvTimer;     //定时器用来判断空闲连接是否超时

    unordered_map<string, TcpAgent*>   m_mapAgents;
};
}