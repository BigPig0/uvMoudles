#pragma once
#include "uv.h"
#include <list>

using namespace std;

namespace uvNetPlus {

enum UV_ASYNC_EVENT
{
    ASYNC_EVENT_TCP_CLIENT = 0, //新建一个tcp客户端
    ASYNC_EVENT_TCP_CONNECT,    //tcp客户端连接
    ASYNC_EVENT_TCP_SEND,       //tcp发送数据
    ASYNC_EVENT_TCP_LISTEN,     //tcp服务端监听
    ASYNC_EVENT_TCP_CLTCLOSE,   //tcp客户端关闭
    ASYNC_EVENT_TCP_SVRCLOSE,   //tcp服务端关闭
    ASYNC_EVENT_TCPCONN_INIT,   //tcp连接池初始化定时器
    ASYNC_EVENT_TCPCONN_REQUEST,//tcp连接池中获取socket
    ASYNC_EVENT_TCPCONN_CLOSE,  //tcp连接池关闭
    ASYNC_EVENT_TCPAGENT_REQUEST, //tcp agent中获取socket
    ASYNC_EVENT_TCP_CONNCLOSE,  //连接池中的socket返还
};

struct UV_EVET {
    UV_ASYNC_EVENT event;
    void* param;
};

struct UV_NODE
{
    bool            m_bRun;
    uv_loop_t       m_uvLoop;
    uv_async_t      m_uvAsync;
    list<UV_EVET>   m_listAsyncEvents;
    uv_mutex_t      m_uvMtxAsEvts;
};


class CUVNetPlus : public CNet
{
public:
    CUVNetPlus();
    ~CUVNetPlus();

    void AddEvent(UV_ASYNC_EVENT e, void* param);

    UV_NODE *pNode;
};

}