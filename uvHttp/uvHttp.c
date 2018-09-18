#include "uvHttp.h"
#include "typedef.h"
#ifdef WIN32
#define fieldcmp _stricmp
#else
#include <strings.h>
#define fieldcmp strcasecmp
#endif

extern void agents_init(http_t* h);
extern void agents_destory(http_t* h);
extern int agents_request(request_p_t* req);
extern void string_map_compare(const void* cpv_first, const void* cpv_second, void* pv_output);

char* url_encode(char* src) {
    size_t len;
    size_t i;
    char* ret;
    string_t* tmp = create_string();
    string_init(tmp);
    len = strlen(src);
    for (i=0; i<len; ++i)
    {
        char c = src[i];
        if(c>='0' && c<='9' || c>='a'&&c<='z' || c>='A' && c<='Z' || c=='-' || c=='_' || c=='.')
        {
            string_push_back(tmp, c);
        }
        //else if(c == ' ')
        //{
        //    string_push_back(ret, '+');
        //}
        else
        {
            char cCode[3]={0};  
            sprintf(cCode,"%02X",c); 
            string_push_back(tmp, '%');
            string_connect_cstr(tmp, cCode);
        }
    }
	len = string_size(tmp);
	ret = (char*)malloc(len+1);
	memcpy(ret, string_c_str(tmp), len);
	ret[len] = 0;
    return ret;
}

char* url_decode(char* src) {
    size_t len;
    char* ret;
    string_t* str_dest = create_string();
    string_init(str_dest);
	len = strlen(src);
    
	len = string_size(str_dest);
	ret = (char*)malloc(len + 1);
	memcpy(ret, string_c_str(str_dest), len);
	ret[len] = 0;
	return ret;
}

#ifdef WIN32
DWORD WINAPI inner_uv_loop_thread(LPVOID lpParam)
{
	http_t* h = (http_t*)lpParam;
	while (h->is_run) {
		uv_run(h->uv, UV_RUN_DEFAULT);
		Sleep(2000);
	}
	uv_loop_close(h->uv);
	free(h);
	return 0;
}
void run_loop_thread(http_t* h)
{
    DWORD threadID = 0;
    HANDLE th = CreateThread(NULL, 0, inner_uv_loop_thread, (LPVOID)h, 0, &threadID);
    CloseHandle(th);
}
#endif

http_t* uvHttp(config_t cof, void* uv) {
    http_t* h = (http_t*)malloc(sizeof(http_t));
    h->conf = cof;
	uv_mutex_init(&h->uv_mutex_h);
	h->is_run = true;
	if (uv != NULL) {
		h->uv = (uv_loop_t*)uv;
		h->inner_uv = false;
	} else {
		h->uv = (uv_loop_t*)malloc(sizeof(uv_loop_t));
		uv_loop_init(h->uv);
		h->inner_uv = true;
#ifdef WIN32
		run_loop_thread(h);
#endif
	}
    agents_init(h);
	return h;
}

void uvHttpClose(http_t* h) {
	agents_destory(h);
	uv_mutex_destroy(&h->uv_mutex_h);
	if (h->inner_uv == true) {
		h->is_run = false;
		uv_stop(h->uv);
	}
	else {
		free(h);
	}
	
}





static const char* http_method[] = {
	"OPTIONS",
	"HEAD",
	"GET",
	"POST",
	"PUT",
	"DELETE",
	"TRACE",
	"CONNECT"
};

static void generic_header(request_p_t* req) {
    map_iterator_t it,end;
	req->str_header = create_string();
	string_init(req->str_header);
	string_connect_cstr(req->str_header, http_method[req->method]);
	string_push_back(req->str_header, ' ');
	string_connect(req->str_header, req->str_path);
	string_connect_cstr(req->str_header, " HTTP/1.1\r\nHost: ");
	string_connect(req->str_header, req->str_host);
	string_connect_cstr(req->str_header, "\r\nConnection: ");
	if (req->keep_alive) {
		string_connect_cstr(req->str_header, "Keep-Alive\r\n");
	} else {
		string_connect_cstr(req->str_header, "Close\r\n");
	}
	if (req->chunked) {
		string_connect_cstr(req->str_header, "Transfer-Encoding: Chunked\r\n");
	} else {
		char clen[20] = { 0 };
		sprintf(clen, "%d", req->content_length);
		string_connect_cstr(req->str_header, "Content-Length: ");
		string_connect_cstr(req->str_header, clen);
		string_connect_cstr(req->str_header, "\r\n");
	}

	if (req->headers != NULL) {
		it = map_begin(req->headers);
		end = map_end(req->headers);
		for (; iterator_not_equal(it, end); it = iterator_next(it))
		{
			pair_t* pt_pair = (pair_t*)iterator_get_pointer(it);
			string_t* name = *(string_t**)pair_first(pt_pair);
			string_t* value = *(string_t**)pair_second(pt_pair);
			string_connect_cstr(req->str_header, string_c_str(name));
			string_connect_cstr(req->str_header, ": ");
			string_connect_cstr(req->str_header, string_c_str(value));
			string_connect_cstr(req->str_header, "\r\n");
		}
	}
	string_connect_cstr(req->str_header, "\r\n");
}

static void on_resolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
	request_p_t* req_p = (request_p_t*)resolver->data;
	uv_mutex_lock(&req_p->uv_mutex_h);
	char addr[17] = { '\0' };
    int err;
	free(resolver);
	if (status < 0) {
		fprintf(stderr, "getaddrinfo callback error %s\n", uv_err_name(status)); 
		if (req_p->req_cb) {
			req_p->req_cb((request_t*)req_p, uv_http_err_dns_parse);
		}
		uv_freeaddrinfo(res);
		free(res);
		return;
	} 
    //保存地址
    req_p->addr = (struct sockaddr*)malloc(sizeof(struct sockaddr));
    memcpy(req_p->addr, res->ai_addr, sizeof(struct sockaddr));
    //解析出ip
	uv_ip4_name((struct sockaddr_in*)res->ai_addr, addr, 16);
	printf("%s\n", addr);
	//此处将原先的域名改为ip
	string_clear(req_p->str_addr);
	string_connect_cstr(req_p->str_addr, addr);

	uv_freeaddrinfo(res);

	generic_header(req_p);
    err = agents_request(req_p);
	if(uv_http_ok != err && req_p->req_cb) {
        req_p->req_cb((request_t*)req_p, err);
    }
	uv_mutex_unlock(&req_p->uv_mutex_h);
}





