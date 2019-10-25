#include "public.h"
#include "private.h"
#include "dns.h"
#include "cstl_easy.h"

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

//////////////////////////////////////////////////////////////////////////

typedef struct _dns_lookup_query_ {
    uv_node_t               *handle;
    string_t                *hostname;
    int                     family;
    int                     hints;
    bool                    all;
    bool                    verbatim;
    dns_lookup_cb           cb;
    void                    *user;
}dns_lookup_query_t;

static void dns_destory_lookup_query(dns_lookup_query_t *query) {
    if(query->hostname)
        string_destroy(query->hostname);
    free(query);
}

static void on_uv_getaddrinfo(uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
    dns_lookup_query_t *query = (dns_lookup_query_t*)req->data;
    free(req);

    if(status < 0) {
        if(query->cb) query->cb(status, NULL, 0, query->user);
        free(query);
        uv_freeaddrinfo(res);
        return;
    }

    //lookup的回调只需要第一个地址
    if(res->ai_family == PF_INET) {
        char addr[17] = {0};
        uv_ip4_name((struct sockaddr_in*)res->ai_addr, addr, 17);
        if(query->cb) query->cb(0,addr,4, query->user);
    } else if(res->ai_family == PF_INET6) {
        char addr[46] = {0};
        uv_ip6_name((struct sockaddr_in6*)res->ai_addr, addr, 46);
        if(query->cb) query->cb(0,addr,6, query->user);
    } else {
        if(query->cb) query->cb(-1, NULL, 0, query->user);
    }

    uv_freeaddrinfo(res);
    dns_destory_lookup_query(query);
}

void dns_lookup_async(void *p ){
    dns_lookup_query_t* query = (dns_lookup_query_t*)p;
    SAFE_MALLOC(uv_getaddrinfo_t, req);
    SAFE_MALLOC(struct addrinfo, hints);
    int ret;
    req->data = query;
    if(query->family == 4)
        hints->ai_family = PF_INET;
    else if(query->family == 6)
        hints->ai_family = PF_INET6;
    else
        hints->ai_family = PF_UNSPEC;

    hints->ai_socktype = SOCK_STREAM;
    hints->ai_protocol = IPPROTO_TCP;
    hints->ai_flags = 0;

    ret = uv_getaddrinfo(query->handle->uv, req, on_uv_getaddrinfo, string_c_str(query->hostname), NULL, hints);
    if (ret < 0) {
        if(query->cb) query->cb(-1, NULL, 0, query->user);
        free(req);
        free(hints);
        dns_destory_lookup_query(query);
    }
}

void dns_lookup(uv_node_t* h, char* hostname, dns_lookup_cb cb, void *user) {
    dns_lookup_options_t options = { 0 };
    dns_lookup_options(h, hostname, options, cb, user);
}

void dns_lookup_family(uv_node_t* h, char* hostname, int family, dns_lookup_cb cb, void *user) {
    dns_lookup_options_t options = { family, 0 };
    dns_lookup_options(h, hostname, options, cb, user);
}

void dns_lookup_options(uv_node_t* h, char* hostname, dns_lookup_options_t options, dns_lookup_cb cb, void *user) {
    uv_thread_t tid = uv_thread_self();
    SAFE_MALLOC(dns_lookup_query_t, query);
    query->handle   = h;
    query->hostname = create_string();
    string_init_cstr(query->hostname, hostname);
    query->family   = options.family;
    query->hints    = options.hints;
    query->all      = options.all;
    query->verbatim = options.verbatim;
    query->cb       = cb;
    query->user     = user;

    if(tid == h->loop_tid) {
        dns_lookup_async(query);
    } else {
        send_async_event(h, ASYNC_EVENT_THREAD_ID, query);
    }
}

void dns_lookup_service(uv_node_t* h, char *address, int port, dns_lookup_service_cb cb, void *user) {

}

//////////////////////////////////////////////////////////////////////////

typedef struct _dns_resolver_ {
    uv_node_t            *handle;
    bool                 canceled;
    bool                 destoried;
    hash_set_t           *servers;          //set<string_t*>
    list_t               *querys;           //list<lookup_query_t*>
    void                 *data;             //用户数据
}dns_resolver_t;

dns_resolver_t* dns_create_resolver(uv_node_t *handle) {
    SAFE_MALLOC(dns_resolver_t, ret);
    ret->handle = handle;
    return ret;
}

void dns_destory_resolver(dns_resolver_t* res) {
    CHECK_POINT_VOID(res);
    res->destoried = true;
    dns_resolver_cancel(res);
}

void dns_set_data(dns_resolver_t* res, void* user_data){
    res->data = user_data;
}

void* dns_get_data(dns_resolver_t* res) {
    return res->data;
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

void dns_set_servers(dns_resolver_t* res, char **servers, int num) {

}