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
    string_t* tmp = create_string();
    string_init(tmp);
    size_t len = strlen(src);
    for (size_t i=0; i<len; ++i)
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
            sprintf_s(cCode, 2,"%02X",c); 
            string_push_back(tmp, '%');
            string_connect_cstr(tmp, cCode);
        }
    }
	len = string_size(tmp);
	char* ret = (char*)malloc(len+1);
	memcpy(ret, string_c_str(tmp), len);
	ret[len] = 0;
    return ret;
}

char* url_decode(char* src) {
    string_t* str_dest = create_string();
    string_init(str_dest);
	size_t len = strlen(src);
    
	len = string_size(str_dest);
	char* ret = (char*)malloc(len + 1);
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
#endif

http_t* uvHttp(config_t cof, void* uv) {
    http_t* h = (http_t*)malloc(sizeof(http_t));
    h->conf = cof;
	h->is_run = true;
	if (uv != NULL) {
		h->uv = (uv_loop_t*)uv;
		h->inner_uv = false;
	} else {
		h->uv = (uv_loop_t*)malloc(sizeof(uv_loop_t));
		uv_loop_init(h->uv);
		h->inner_uv = true;
#ifdef WIN32
		DWORD threadID = 0;
		HANDLE th = CreateThread(NULL, 0, inner_uv_loop_thread, (LPVOID)h, 0, &threadID);
		CloseHandle(th);
#endif
	}
    agents_init(h);
	return h;
}

void uvHttpClose(http_t* h) {
	agents_destory(h);
	if (h->inner_uv == true) {
		h->is_run = false;
		uv_stop(h->uv);
	}
	else {
		free(h);
	}
	
}

request_t* creat_request(http_t* h, request_cb req_cb, response_data res_data, response_cb res_cb) {
	request_p_t* req = (request_p_t*)malloc(sizeof(request_t));
    memset(req, 0 , sizeof(request_p_t));
	req->keep_alive = 1;
    req->handle = h;
    req->req_cb = req_cb;
    req->res_data = res_data;
    req->res_cb = res_cb;

	return (request_t*)req;
}

void add_req_header(request_t* req, const char* key, const char* value) {
	request_p_t* req_p = (request_p_t*)req;
	if (!fieldcmp("Content-Length", key)) {
		req_p->content_length = atoi(value);
		return;
	}
	if (!fieldcmp("Connection", key)) {
		if (!fieldcmp("Close", value)) {
			req_p->keep_alive = 0;
		}
		return;
	}
	if (!fieldcmp("Transfer-Encoding", key)) {
		if (!fieldcmp("chunked", value)) {
			req_p->chunked = 1;
		}
		return;
	}
	if (req_p->headers == NULL) {
		req_p->headers = create_map(void*, void*);
		map_init_ex(req_p->headers, string_map_compare);
	}
	string_t* str_key = create_string();
	string_init_cstr(str_key, key);
	map_iterator_t it_pos = map_find(req_p->headers, str_key);
	if (iterator_equal(it_pos, map_end(req_p->headers))) {
		string_t* str_value = create_string();
		string_init_cstr(str_value, value);
		pair_t* pt_pair = create_pair(void*, void*);
		pair_init_elem(pt_pair, str_key, str_value);
		map_insert(req_p->headers, pt_pair);
		pair_destroy(pt_pair);
	} else {
		pair_t* pt_pair = (pair_t*)iterator_get_pointer(it_pos);
		string_t* str_value = *(string_t**)pair_second(pt_pair);
		string_clear(str_value);
		string_copy(str_value, (char*)value, strlen(value), 0);
		string_destroy(str_key);
	}
}

int add_req_body(request_t* req, const char* data, int len) {
	request_p_t* req_p = (request_p_t*)req;
	membuff_t buf;
	buf.data = (unsigned char*)data;
	buf.len = len;
	if (req_p->body == NULL) {
		req_p->body = create_list(membuff_t);
		list_init_elem(req_p->body, 1, buf);
	} else {
		list_push_back(req_p->body, buf);
	}
	return uv_http_ok;
}

int get_res_header_count(response_t* res) {
	response_p_t* res_p = (response_p_t*)res;
	return map_size(res_p->headers);
}

char* get_res_header_name(response_t* res, int i) {
	response_p_t* res_p = (response_p_t*)res;
	pair_t* pt_pair = (pair_t*)map_at(res_p->headers, i);
	string_t* str_name = *(string_t**)pair_first(pt_pair);
	return (char*)string_c_str(str_name);
}

char* get_res_header_value(response_t* res, int i) {
	response_p_t* res_p = (response_p_t*)res;
	pair_t* pt_pair = (pair_t*)map_at(res_p->headers, i);
	string_t* str_value = *(string_t**)pair_second(pt_pair);
	return (char*)string_c_str(str_value);
}

char* get_res_header(response_t* res, const char* key) {
	response_p_t* res_p = (response_p_t*)res;
	string_t* str_key = create_string();
	string_init_cstr(str_key, key);
	map_iterator_t it_pos = map_find(res_p->headers, str_key);
	if (iterator_equal(it_pos, map_end(res_p->headers))) {
		return NULL;
	} else {
		pair_t* pt_pair = (pair_t*)iterator_get_pointer(it_pos);
		string_t* str_value = *(string_t**)pair_second(pt_pair);
		return (char*)string_c_str(str_value);
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
		string_connect_cstr(req->str_header, "Content-Length: ");
		char clen[20] = { 0 };
		sprintf(clen, "%d", req->content_length);
		string_connect_cstr(req->str_header, clen);
		string_connect_cstr(req->str_header, "\r\n");
	}

	map_iterator_t it = map_begin(req->headers);
	map_iterator_t end = map_end(req->headers);
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
	string_connect_cstr(req->str_header, "\r\n");
}

static void on_resolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
	request_p_t* req_p = (request_p_t*)resolver->data;
	free(resolver);
	if (status < 0) {
		fprintf(stderr, "getaddrinfo callback error %s\n", uv_err_name(status)); 
		if (req_p->req_cb) {
			req_p->req_cb(uv_http_err_dns_parse, (request_t*)req_p);
		}
		uv_freeaddrinfo(res);
		free(res);
		return;
	} 
    req_p->addr = res->ai_addr;
	char addr[17] = { '\0' };
	uv_ip4_name((struct sockaddr_in*)res->ai_addr, addr, 16);
	printf("%s\n", addr);
	//此处将原先的域名改为ip
	string_clear(req_p->str_addr);
	string_connect_cstr(req_p->str_addr, addr);

	uv_freeaddrinfo(res);
	free(res);

	generic_header(req_p);
    int err = agents_request(req_p);
	if(uv_http_ok != err && req_p->req_cb) {
        req_p->req_cb(err, (request_t*)req_p);
    }
}

int request(request_t* req) {
	request_p_t* req_p = (request_p_t*)req;
	//解析url
	string_t* str_url = create_string();
	string_init_cstr(str_url, req_p->url);
	size_t pos = string_find_cstr(str_url, "://", 0);
	size_t addr_begin;
	if (pos == NPOS) {
		//没有填写协议，默认为http协议
		addr_begin = 0;
	} else {
		if (0 != string_compare_substring_cstr(str_url, 0, 7, "http://")) {
			return uv_http_err_protocol;
		}
		addr_begin = 7;
	}
	size_t path_begin = string_find_char(str_url, '/', addr_begin);
	req_p->str_host = create_string();
	req_p->str_path = create_string();
	if (path_begin == NPOS) {
		//未填写path，默认根目录
		if (req_p->host == NULL) {
			//未填写host
			string_init_copy_substring(req_p->str_host, str_url, addr_begin, path_begin - addr_begin);
		} else {
			//指定host
			string_init_cstr(req_p->str_host, req_p->host);
		}
		string_init_cstr(req_p->str_path, "/");
	} else {
		string_init_copy_substring(req_p->str_path, str_url, path_begin, string_size(str_url) - path_begin);
	}
	string_destroy(str_url);

	//解析出host中的域名地址和端口
	req_p->str_addr = create_string();
	req_p->str_port = create_string();
	size_t port_begin = string_find_char(req_p->str_host, ':', 0);
	if (port_begin == NPOS) {
		//默认端口
		string_init_copy(req_p->str_addr, req_p->str_host);
		string_init_cstr(req_p->str_port, "80");
	} else {
		string_init_copy_substring(req_p->str_addr, req_p->str_host, 0, port_begin);
		string_init_copy_substring(req_p->str_port, req_p->str_host, port_begin+1, string_size(req_p->str_host)- port_begin-1);
	}

	uv_getaddrinfo_t* resolver = (uv_getaddrinfo_t*)malloc(sizeof(uv_getaddrinfo_t));
	resolver->data = req_p;
	struct addrinfo *hints = (struct addrinfo *)malloc(sizeof(struct addrinfo));
	hints->ai_family = PF_INET;
	hints->ai_socktype = SOCK_STREAM;
	hints->ai_protocol = IPPROTO_TCP;
	hints->ai_flags = 0;
	int r = uv_getaddrinfo(req_p->handle->uv, resolver, on_resolved, string_c_str(req_p->str_addr), string_c_str(req_p->str_port), hints);
	if (r < 0) {
		return uv_http_err_dns_parse;
	}

	//后续操作在dns解析的回调中执行
    return 0;
}

int request_write(request_t* req, char* data, int len) {
	return uv_http_ok;
}
