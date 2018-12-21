#include "public_def.h"
#include "private_def.h"
#include "dns.h"
#include "cstl_easy.h"


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


/////////////////////////////////////////////////////////////////
char* dns_strerror(int err) {
    static char *dns_error[] = {
        "success",
        "DNS server returned answer with no data.",
        "DNS server claims query was misformatted.",
        "DNS server returned general failure.",
        "Domain name not found.",
        "DNS server does not implement requested operation.",
        "DNS server refused query.",
        "Misformatted DNS query.",
        "Misformatted hostname.",
        "Unsupported address family.",
        "Misformatted DNS reply.",
        "Could not contact DNS servers.",
        "Timeout while contacting DNS servers.",
        "End of file.",
        "Error reading file.",
        "Out of memory.",
        "Channel is being destroyed.",
        "Misformatted string.",
        "Illegal flags specified.",
        "Given hostname is not numeric.",
        "Illegal hints flags specified.",
        "c-ares library initialization not yet performed.",
        "Error loading iphlpapi.dll.",
        "Could not find GetNetworkParams function.",
        "DNS query cancelled."
    };
    if(err < DNS_ERROR_MAX && err >= 0) {
        return dns_error[err];
    }
    return NULL;
}

typedef struct _lookup_query_ {
    char                    *hostname;
    dns_lookup_options_t    *options;
    dns_lookup_cb           cb;
    list_t                  jobs;          //list<>
    list_t                  answers;       //list<>
    dns_resolver_t          *resolver;
}lookup_query_t;

typedef struct _dns_resolver_ {
    http_t               *handle;
    bool                 canceled;
    bool                 destoried;
    hash_set_t           *servers;          //set<string_t*>
    list_t               *querys;           //list<lookup_query_t*>
}dns_resolver_t;

dns_lookup_options_t* dns_create_lookup_options() {
    SAFE_MALLOC(dns_lookup_options_t, ret);
    return ret;
}

dns_resolver_t* dns_create_resolver(http_t *handle) {
    SAFE_MALLOC(dns_resolver_t, ret);
    ret->handle = handle;
    return ret;
}

void dns_destory_resolver(dns_resolver_t* res) {
    CHECK_POINT_VOID(res);
    res->destoried = true;
    dns_resolver_cancel(res);
}

void dns_resolver_cancel(dns_resolver_t* res) {
    CHECK_POINT_VOID(res);
    res->canceled = true;
}

int dns_resolver_get_servers(dns_resolver_t* res, char** servers) {
    int i = 0;
    CHECK_POINT_INT(res, 0);
    CHECK_POINT_INT(res->servers, 0);
    servers = (char**)calloc(sizeof(char*), hash_set_size(res->servers));
    HASH_SET_FOR_BEGIN(res->servers, string_t*, server)
        servers[i++] = (char*)string_c_str(server);
    HASH_SET_FOR_END
    return hash_set_size(res->servers);
}

void dns_lookup(dns_resolver_t* res, char* hostname, dns_lookup_cb cb) {
    int ret;
    SAFE_MALLOC(lookup_query_t, query);
    SAFE_MALLOC(uv_getaddrinfo_t, resolver);
    SAFE_MALLOC(struct addrinfo, hints);

    query->resolver = res;
    query->cb = cb;

    resolver->data = query;

    hints->ai_family = PF_INET;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_protocol = IPPROTO_TCP;
    hints->ai_flags = 0;

    ret = uv_getaddrinfo(res->handle->uv, resolver, on_resolved, hostname, NULL, hints);
    if (ret < 0) {
        free(resolver);
        free(hints);
    }
}

void dns_lookup_options(dns_resolver_t* res, char* hostname, dns_lookup_options_t *options, dns_lookup_cb cb) {
    SAFE_MALLOC(lookup_query_t, query);
    query->resolver = res;
    query->options = options;
    query->cb = cb;
}

void dns_lookup_family(dns_resolver_t* res, char* hostname, int family, dns_lookup_cb cb) {
    SAFE_MALLOC(lookup_query_t, query);
    SAFE_MALLOC(dns_lookup_options_t, options);
    query->resolver = res;
    options->family = family;
    query->options = options;
    query->cb = cb;
}

void dns_lookup_service(dns_resolver_t* res, char *address, int port, dns_lookup_service_cb cb) {

}

void dns_set_servers(dns_resolver_t* res, char **servers, int num) {

}