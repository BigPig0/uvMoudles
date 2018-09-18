#include "public_type.h"
#include "typedef.h"

int get_res_header_count(response_t* res) {
	response_p_t* res_p = (response_p_t*)res;
	uv_mutex_lock(&res_p->uv_mutex_h);
	int ret = map_size(res_p->headers);
	uv_mutex_unlock(&res_p->uv_mutex_h);
	return ret;
}

char* get_res_header_name(response_t* res, int i) {
	response_p_t* res_p = (response_p_t*)res;
	uv_mutex_lock(&res_p->uv_mutex_h);
	pair_t* pt_pair = (pair_t*)map_at(res_p->headers, i);
	string_t* str_name = *(string_t**)pair_first(pt_pair);
	uv_mutex_unlock(&res_p->uv_mutex_h);
	return (char*)string_c_str(str_name);
}

char* get_res_header_value(response_t* res, int i) {
	response_p_t* res_p = (response_p_t*)res;
	uv_mutex_lock(&res_p->uv_mutex_h);
	pair_t* pt_pair = (pair_t*)map_at(res_p->headers, i);
	string_t* str_value = *(string_t**)pair_second(pt_pair);
	uv_mutex_unlock(&res_p->uv_mutex_h);
	return (char*)string_c_str(str_value);
}

char* get_res_header(response_t* res, const char* key) {
	response_p_t* res_p = (response_p_t*)res;
	uv_mutex_lock(&res_p->uv_mutex_h);
	string_t* str_key = create_string();
	map_iterator_t it_pos;
	string_init_cstr(str_key, key);
	it_pos = map_find(res_p->headers, str_key);
	if (iterator_equal(it_pos, map_end(res_p->headers))) {
		uv_mutex_unlock(&res_p->uv_mutex_h);
		return NULL;
	}
	else {
		pair_t* pt_pair = (pair_t*)iterator_get_pointer(it_pos);
		string_t* str_value = *(string_t**)pair_second(pt_pair);
		uv_mutex_unlock(&res_p->uv_mutex_h);
		return (char*)string_c_str(str_value);
	}
	uv_mutex_unlock(&res_p->uv_mutex_h);
}

static void recive_response(request_p_t* req, char* data, int len) {
	response_p_t* res = req->res;
	if (req->res == NULL) {
		req->res = (response_p_t*)malloc(sizeof(response_p_t));
		memset(req->res, 0, sizeof(response_p_t));
		req->res->req = req;
		req->res->handle = req->handle;
		req->res->headers = create_map(void*, void*); //map_t<string_t*,string_t*>
		map_init(req->res->headers);
	}
}