#include "public_type.h"
#include "typedef.h"

request_t* creat_request(http_t* h, request_cb req_cb, response_data res_data, response_cb res_cb) {
	request_p_t* req = (request_p_t*)malloc(sizeof(request_t));
	memset(req, 0, sizeof(request_p_t));
	req->keep_alive = 1;
	req->handle = h;
	req->req_cb = req_cb;
	req->res_data = res_data;
	req->res_cb = res_cb;
	uv_mutex_init(&req->uv_mutex_h);

	return (request_t*)req;
}

void add_req_header(request_t* req, const char* key, const char* value) {
	string_t* str_key;
	map_iterator_t it_pos;
	request_p_t* req_p = (request_p_t*)req;
	uv_mutex_lock(&req_p->uv_mutex_h);
	if (!fieldcmp("Content-Length", key)) {
		req_p->content_length = atoi(value);
		uv_mutex_unlock(&req_p->uv_mutex_h);
		return;
	}
	if (!fieldcmp("Connection", key)) {
		if (!fieldcmp("Close", value)) {
			req_p->keep_alive = 0;
		}
		uv_mutex_unlock(&req_p->uv_mutex_h);
		return;
	}
	if (!fieldcmp("Transfer-Encoding", key)) {
		if (!fieldcmp("chunked", value)) {
			req_p->chunked = 1;
		}
		uv_mutex_unlock(&req_p->uv_mutex_h);
		return;
	}
	if (req_p->headers == NULL) {
		req_p->headers = create_map(void*, void*);
		map_init_ex(req_p->headers, string_map_compare);
	}
	str_key = create_string();
	string_init_cstr(str_key, key);
	it_pos = map_find(req_p->headers, str_key);
	if (iterator_equal(it_pos, map_end(req_p->headers))) {
		string_t* str_value;
		pair_t* pt_pair;
		str_value = create_string();
		string_init_cstr(str_value, value);
		pt_pair = create_pair(void*, void*);
		pair_init_elem(pt_pair, str_key, str_value);
		map_insert(req_p->headers, pt_pair);
		pair_destroy(pt_pair);
	}
	else {
		pair_t* pt_pair;
		string_t* str_value;
		pt_pair = (pair_t*)iterator_get_pointer(it_pos);
		str_value = *(string_t**)pair_second(pt_pair);
		string_clear(str_value);
		string_copy(str_value, (char*)value, strlen(value), 0);
		string_destroy(str_key);
	}
	uv_mutex_unlock(&req_p->uv_mutex_h);
}

int add_req_body(request_t* req, const char* data, int len) {
	request_p_t* req_p = (request_p_t*)req;
	uv_mutex_lock(&req_p->uv_mutex_h);
	membuff_t buf;
	buf.data = (unsigned char*)data;
	buf.len = len;
	if (req_p->body == NULL) {
		req_p->body = create_list(membuff_t);
		list_init_elem(req_p->body, 1, buf);
	}
	else {
		list_push_back(req_p->body, buf);
	}
	uv_mutex_unlock(&req_p->uv_mutex_h);
	return uv_http_ok;
}

int request(request_t* req) {
	request_p_t* req_p = (request_p_t*)req;
	uv_mutex_lock(&req_p->uv_mutex_h);
	size_t pos, addr_begin, path_begin, port_begin;
	uv_getaddrinfo_t* resolver;
	struct addrinfo *hints;
	int r;
	//解析url
	string_t* str_url = create_string();
	string_init_cstr(str_url, req_p->url);
	pos = string_find_cstr(str_url, "://", 0);
	if (pos == NPOS) {
		//没有填写协议，默认为http协议
		addr_begin = 0;
	}
	else {
		if (0 != string_compare_substring_cstr(str_url, 0, 7, "http://")) {
			return uv_http_err_protocol;
		}
		addr_begin = 7;
	}
	path_begin = string_find_char(str_url, '/', addr_begin);
	req_p->str_host = create_string();
	req_p->str_path = create_string();
	if (path_begin == NPOS) {
		//未填写path，默认根目录
		if (req_p->host == NULL) {
			//未填写host
			string_init_copy_substring(req_p->str_host, str_url, addr_begin, path_begin - addr_begin);
		}
		else {
			//指定host
			string_init_cstr(req_p->str_host, req_p->host);
		}
		string_init_cstr(req_p->str_path, "/");
	}
	else {
		string_init_copy_substring(req_p->str_path, str_url, path_begin, string_size(str_url) - path_begin);
	}
	string_destroy(str_url);

	//解析出host中的域名地址和端口
	req_p->str_addr = create_string();
	req_p->str_port = create_string();
	port_begin = string_find_char(req_p->str_host, ':', 0);
	if (port_begin == NPOS) {
		//默认端口
		string_init_copy(req_p->str_addr, req_p->str_host);
		string_init_cstr(req_p->str_port, "80");
	}
	else {
		string_init_copy_substring(req_p->str_addr, req_p->str_host, 0, port_begin);
		string_init_copy_substring(req_p->str_port, req_p->str_host, port_begin + 1, string_size(req_p->str_host) - port_begin - 1);
	}

	resolver = (uv_getaddrinfo_t*)malloc(sizeof(uv_getaddrinfo_t));
	resolver->data = req_p;
	hints = (struct addrinfo *)malloc(sizeof(struct addrinfo));
	hints->ai_family = PF_INET;
	hints->ai_socktype = SOCK_STREAM;
	hints->ai_protocol = IPPROTO_TCP;
	hints->ai_flags = 0;
	r = uv_getaddrinfo(req_p->handle->uv, resolver, on_resolved, string_c_str(req_p->str_addr), string_c_str(req_p->str_port), hints);
	if (r < 0) {
		free(resolver);
		free(hints);
		uv_mutex_unlock(&req_p->uv_mutex_h);
		return uv_http_err_dns_parse;
	}

	//后续操作在dns解析的回调中执行
	uv_mutex_unlock(&req_p->uv_mutex_h);
	return uv_http_ok;
}

int request_write(request_t* req, char* data, int len) {
	return uv_http_ok;
}

void destory_request(request_p_t* req) {
	response_p_t*  res = req->res;
	if (NULL != res) {
		if (NULL != res->headers) {
			map_iterator_t it = map_begin(res->headers);
			map_iterator_t end = map_end(res->headers);
			for (; iterator_not_equal(it, end); it = iterator_next(it))
			{
				pair_t* pt_pair = (pair_t*)iterator_get_pointer(it);
				string_t* key = *(string_t**)pair_first(pt_pair);
				string_t* value = *(string_t**)pair_second(pt_pair);
				string_destroy(key);
				string_destroy(value);
			}
			map_destroy(res->headers);
		}

		free(res);
	}

	uv_mutex_lock(&req->uv_mutex_h);
	string_destroy(req->str_host);
	string_destroy(req->str_addr);
	string_destroy(req->str_port);
	string_destroy(req->str_path);
	if (req->addr) free(req->addr);
	string_destroy(req->str_header);

	if (req->headers) {
		map_iterator_t it = map_begin(req->headers);
		map_iterator_t end = map_end(req->headers);
		for (; iterator_not_equal(it, end); it = iterator_next(it))
		{
			pair_t* pt_pair = (pair_t*)iterator_get_pointer(it);
			string_t* key = *(string_t**)pair_first(pt_pair);
			string_t* value = *(string_t**)pair_second(pt_pair);
			string_destroy(key);
			string_destroy(value);
		}
		map_destroy(req->headers);
	}

	if (req->body)
		list_destroy(req->body); //body内容由外部自己管理

	uv_mutex_unlock(&req->uv_mutex_h);
	uv_mutex_destroy(&req->uv_mutex_h);
	//free(req);
}