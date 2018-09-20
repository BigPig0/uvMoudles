#include "public_def.h"
#include "private_def.h"

response_p_t* create_response(request_p_t* req) {
    response_p_t* res = (response_p_t*)malloc(sizeof(response_p_t));
    memset(res, 0, sizeof(response_p_t));
    res->req = req;
    res->handle = req->handle;
    res->headers = create_map(void*, void*); //map_t<string_t*,string_t*>
    map_init(res->headers);

    return res;
}
int get_res_header_count(response_t* res) {
	response_p_t* res_p = (response_p_t*)res;
	int ret = map_size(res_p->headers);
	return ret;
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
	map_iterator_t it_pos;
	string_init_cstr(str_key, key);
	it_pos = map_find(res_p->headers, str_key);
	if (iterator_equal(it_pos, map_end(res_p->headers))) {
		return NULL;
	}
	else {
		pair_t* pt_pair = (pair_t*)iterator_get_pointer(it_pos);
		string_t* str_value = *(string_t**)pair_second(pt_pair);
		return (char*)string_c_str(str_value);
	}
}

void recive_response(response_p_t* res, char* data, int len) {
    printf("recive_response,%s len:%d\r\n", res->req->url, len);
	if (res == NULL) {
		return;
	}
}

void destory_response(response_p_t* res) {
    if (NULL == res) {
        return;
    }
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