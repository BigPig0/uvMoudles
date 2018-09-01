#include "uvHttp.h"
#include "typedef.h"

extern void agents_init(http_t* h);
extern void agents_destory(http_t* h);
extern agent_t* get_agent(http_t* h, string_t* addr);
extern int creat_request(agent_t* agent, option_t* o);

string_t* url_encode(string_t* src) {
    string_t* ret = create_string();
    string_init(ret);
    size_t len = string_size(src);
    for (size_t i=0; i<len; ++i)
    {
        char c = *string_at(src, i);
        if(c>='0' && c<='9' || c>='a'&&c<='z' || c>='A' && c<='Z' || c=='-' || c=='_' || c=='.')
        {
            string_push_back(ret, c);
        }
        //else if(c == ' ')
        //{
        //    string_push_back(ret, '+');
        //}
        else
        {
            char cCode[3]={0};  
            sprintf_s(cCode,"%02X",c); 
            string_push_back(ret, '%');
            string_connect_cstr(ret, cCode);
        }
    }
    return ret;
}

string_t* url_decode(string_t* src) {
    string_t* ret = create_string();
    string_init(ret);
    return ret;
}

http_t* uvHttp(config_t cof) {
    http_t* h = (http_t*)malloc(sizeof(http_t));
    h->conf = cof;
    agents_init(h);
}

void ubHttpClose(http_t* h) {
    agents_destory(h);
    free(h);
}

request_t* creat_request(http_t* h) {
}

int request(request_t* req, request_cb cb) {
    string_t* addr = create_string();
    string_init(addr);
    string_connect_cstr(addr, req->host);
    string_connect_char(addr, ':');
    string_connect_cstr(addr, req->port);
    agent_t* aggent = get_agent(req->handle, addr);
    string_destroy(addr);
    return creat_request(aggent, o);
}

int request_write(request_t* req, char* data, int len, req_write_cb cb) {

}

int on_response_data(response_t* res, response_data cb) {

}

int on_response_end(response_t* res, response_end cb) {

}