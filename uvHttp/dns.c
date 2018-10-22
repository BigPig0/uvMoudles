#include "public_def.h"
#include "private_def.h"

static void on_resolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
    request_p_t* req_p = (request_p_t*)resolver->data;
    char addr[17] = { '\0' };
    int err;
    free(resolver);
    uv_mutex_lock(&req_p->uv_mutex_h);
    do {
        if (status < 0) {
            fprintf(stderr, "getaddrinfo callback error %s\n", uv_err_name(status)); 
            if (req_p->req_cb) {
                req_p->req_cb((request_t*)req_p, uv_http_err_dns_parse);
            }
            break;
        } 
        //保存地址
        req_p->addr = (struct sockaddr*)malloc(sizeof(struct sockaddr));
        memcpy(req_p->addr, res->ai_addr, sizeof(struct sockaddr));
        //解析出ip
        uv_ip4_name((struct sockaddr_in*)res->ai_addr, addr, 16);
        printf("get ip from dns: %s\n", addr);
        //此处将原先的域名改为ip
        string_clear(req_p->str_addr);
        string_connect_cstr(req_p->str_addr, addr);

        //状态更新
        req_p->req_time = time(NULL);
        req_p->req_step = request_step_dns;

        //交由agent发送请求
        err = agent_request(req_p);
        if(uv_http_ok != err && req_p->req_cb) {
            req_p->req_cb((request_t*)req_p, err);
            break;
        }
    } while(0);

    uv_freeaddrinfo(res);
    uv_mutex_unlock(&req_p->uv_mutex_h);
}

int parse_dns(request_p_t* req) {
    uv_getaddrinfo_t* resolver;
    struct addrinfo *hints;
    int ret;

    resolver = (uv_getaddrinfo_t*)malloc(sizeof(uv_getaddrinfo_t));
    resolver->data = req;
    hints = (struct addrinfo *)malloc(sizeof(struct addrinfo));
    hints->ai_family = PF_INET;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_protocol = IPPROTO_TCP;
    hints->ai_flags = 0;
    ret = uv_getaddrinfo(req->handle->uv, resolver, on_resolved, string_c_str(req->str_addr), string_c_str(req->str_port), hints);
    if (ret < 0) {
        free(resolver);
        free(hints);
        return uv_http_err_dns_parse;
    }

    return uv_http_ok;
}