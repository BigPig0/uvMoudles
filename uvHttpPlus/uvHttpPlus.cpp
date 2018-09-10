#include "uvHttpPlus.h"

/**
 * 
 */
CResponsePluse::CResponsePluse()
{
}

CResponsePluse::~CResponsePluse()
{
}


/**
 * 
 */
CRequest::CRequest()
{
}

CRequest::~CRequest()
{
}

void CRequest::SetMethod(HTTP_METHOD eMethod)
{
    m_pReq->method = eMethod;
}
void CRequest::SetURL(string strURL)
{
    m_strUrl = strURL;
    m_pReq->url = m_strUrl.c_str();
}
void CRequest::SetHost(string strHost)
{
    m_strHost = strHost;
    m_pReq->host = m_strHost.c_str();
}
void CRequest::SetKeepAlive(bool bKeepAlive)
{
    m_pReq->keep_alive = bKeepAlive?1:0;
}
void CRequest::SetChunked(bool bChunked)
{
    m_pReq->chunked = bChunked?1:0;
}
void CRequest::SetContentLength(int nLength)
{
    m_pReq->content_length = nLength;
}
void CRequest::SetUserData(void* pData)
{
    m_pUserData = pData;
}

void CRequest::AddHeader(string strName, string strValue)
{
    add_req_header(m_pReq, strName.c_str(), strValue.c_str());
}

void CRequest::AddBody(char* pBuff, int nLen)
{
    add_req_body(m_pReq, pBuff, nLen);
}

int CRequest::Request()
{
    return request(m_pReq);
}

void CRequest::RequestCB(request_t* req, int code)
{
    if(m_funReqCb)
        m_funReqCb(req, code);
}

void CRequest::ResponseData(request_t* req, char* data, int len)
{
    if(m_funResData)
        m_funResData(req, data, len);
}

void CRequest::ResponseCB(request_t* req, int code)
{
    if(m_funResCb)
        m_funResCb(req, code);
}

void plus_request_cb(request_t* req, int code)
{
    CRequest* pRequest = (CRequest*)req->user_data;
    pRequest->RequestCB(req, code);
    if (code > 0)
        delete pRequest;
}

void plus_response_data(request_t* req, char* data, int len)
{
    CRequest* pRequest = (CRequest*)req->user_data;
    pRequest->ResponseData(req, data, len);
}

void pluse_response_cb(request_t* req, int code)
{
    CRequest* pRequest = (CRequest*)req->user_data;
    pRequest->ResponseCB(req, code);
    if(code > 0)
        delete pRequest;
}

/**
 * 
 */
CHttpPlus::CHttpPlus(config_t cof, void* uv)
{
    m_pHadle = uvHttp(cof, uv);
}

CHttpPlus::~CHttpPlus()
{
    uvHttpClose(m_pHadle);
}

CRequest* CHttpPlus::CreatRequest(request_cb_plus req_cb, response_data_plus res_data, response_cb_plus res_cb)
{
    request_t* req = creat_request(m_pHadle, plus_request_cb, plus_response_data, pluse_response_cb);
    CRequest* pRequest = new CRequest();
    req->user_data = pRequest;
    pRequest->m_pReq = req;
    pRequest->m_funReqCb = req_cb;
    pRequest->m_funResData = res_data;
    pRequest->m_funResCb = res_cb;
    return pRequest;
}