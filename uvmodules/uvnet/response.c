#include "public.h"
#include "private.h"

/* 十六进制字符串转为数字*/
static int htoi(char *s) {
    int i;
    int ret = 0;
    int len = strlen(s);
    for (i = 0; i<len; ++i) {
        if (s[i] >= 'a' && s[i] <= 'f') {
            ret = 16 * ret + (10 + tolower(s[i]) - 'a');
        } else if (s[i] >= 'A' && s[i] <= 'F') {
            ret = 16 * ret + (10 + tolower(s[i]) - 'A');
        } else if (s[i] >= '0' && s[i] <= '9') {
            ret = 16 * ret + (tolower(s[i]) - '0');
        } 
    }
    return ret;
}

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
    } else {
        pair_t* pt_pair = (pair_t*)iterator_get_pointer(it_pos);
        string_t* str_value = *(string_t**)pair_second(pt_pair);
        return (char*)string_c_str(str_value);
    }
}

bool_t response_recive(response_p_t* res, char* data, int len, int errcode) {
    bool_t finish = false;
    int i = 0;
    string_t* str_tmp = create_string();
    string_t* str_tmp2 = create_string();
    string_init(str_tmp);
    string_init(str_tmp2);
    printf("response_recive,%s len:%d\r\n", res->req->url, len);

    for (; i < len; ++i) {
        if (response_step_protocol == res->parse_step) {
            if (data[i] == '/') {
                if (string_compare_substring_cstr(str_tmp, string_size(str_tmp) - 4, 4, "http") != 0) {
                    res->parse_step = response_step_version;
                } else {
                    res->parse_step = response_step_status_code;
                }
                string_clear(str_tmp);
            } else {
                string_push_back(str_tmp, data[i]);
            }
        } else if (response_step_version == res->parse_step) {
            if (data[i] == ' ') {
                res->vesion = str_tmp;
                str_tmp = create_string();
                string_init(str_tmp);
                res->parse_step = response_step_status_code;
            } else {
                string_push_back(str_tmp, data[i]);
            }
        } else if (response_step_status_code == res->parse_step) {
            if (data[i] == ' ') {
                res->status = atoi(string_c_str(str_tmp));
                string_clear(str_tmp);
                res->parse_step = response_step_status_desc;
            } else {
                string_push_back(str_tmp, data[i]);
            }
        } else if (response_step_status_desc == res->parse_step) {
            if (data[i] == '\r' && data[i + 1] == '\n') {
                res->status_desc = str_tmp;
                str_tmp = create_string();
                string_init(str_tmp);
                res->parse_step = response_step_header_key;
                i++;
            } else {
                string_push_back(str_tmp, data[i]);
            }
        } else if (response_step_header_key == res->parse_step) {
            if (data[i] == ':') {
                res->parse_step = response_step_header_value;
            } else {
                string_push_back(str_tmp, data[i]);
            }
        } else if (response_step_header_value == res->parse_step) {
            if (data[i] == '\r' && data[i + 1] == '\n') {
                pair_t* pt_pair = create_pair(void*, void*);
                pair_init_elem(pt_pair, str_tmp, str_tmp2);
                map_insert(res->headers, pt_pair);
                pair_destroy(pt_pair);
                if (!strcasecmp(string_c_str(str_tmp), "Connection")) {
                    if (!strcasecmp(string_c_str(str_tmp2), "Close")) {
                        res->keep_alive = 0;
                    } else {
                        res->keep_alive = 1;
                    }
                } else if (!strcasecmp(string_c_str(str_tmp), "Transfer-Encoding")) {
                    if (!strcasecmp(string_c_str(str_tmp2), "chunked")) {
                        res->chunked = 1;
                        printf("chunked\r\n");
                    }
                } else if (!strcasecmp(string_c_str(str_tmp), "Content-Length")) {
                    res->content_length = atoi(string_c_str(str_tmp2));
                    printf("content length:%d\r\n", res->content_length);
                }
                str_tmp = create_string();
                str_tmp2 = create_string();
                string_init(str_tmp);
                string_init(str_tmp2);
                if (data[i + 2] == '\r' && data[i + 3] == '\n') {
                    if (res->chunked) {
                        res->parse_step = response_step_chunk_len;
                    } else {
                        res->parse_step = response_step_body;
                    }
                    i += 3;
                } else {
                    res->parse_step = response_step_header_key;
                    i++;
                }
            } else {
                if (data[i] != ' ' || !string_empty(str_tmp2)) {
                    string_push_back(str_tmp2, data[i]);
                }
            }
        } else if (response_step_body == res->parse_step) {
            int recived = res->recived_length + (len - i);
            if (res->content_length > 0 && recived > res->content_length) {
                int need = res->content_length - res->recived_length;
                res->req->res_data((request_t*)res->req, data + i, need);
            } else {
                res->req->res_data((request_t*)res->req, data + i, len - i);
            }
            res->recived_length = recived;
            printf("recived:%d, content:%d\r\n", recived, res->content_length);
            if (res->content_length > 0 && res->recived_length >= res->content_length) {
                res->req->res_cb((request_t*)res->req, uv_http_ok);
                finish = true;
            }
            break;
        } else if (response_step_chunk_len == res->parse_step) {
            if (data[i] == '\r' && data[i + 1] == '\n') {
                res->chunk_length = htoi((char*)string_c_str(str_tmp));
                printf("this chunk length is: %X, %d\r\n", res->chunk_length, res->chunk_length);
                if (0 == res->chunk_length) {
                    printf("chunk receive finish\r\n");
                    res->req->res_cb((request_t*)res->req, uv_http_ok);
                    finish = true;
                    break;
                }
                res->recived_length = 0;
                string_clear(str_tmp);
                res->parse_step = response_step_chunk_buff;
                i++;
            } else {
                string_push_back(str_tmp, data[i]);
            }
        } else if (response_step_chunk_buff == res->parse_step) {
            int recived = res->recived_length + (len - i);
            if (recived > res->chunk_length) {
                int need = res->chunk_length - res->recived_length;
                if(i > 0)
                    res->req->res_data((request_t*)res->req, data + i, need);
                printf("a chunk is end: %x, %x\r\n", data[i + need + 1], data[i + need + 2]);
                i += (need + 2);
                res->parse_step = response_step_chunk_len;
            } else {
                res->req->res_data((request_t*)res->req, data + i, len - i);
                res->recived_length = recived;
                break;
            }
        }
    }
    return finish;
}

void response_error(response_p_t* res, int code) {
    printf("response_error: %s\r\n", uvhttp_err_msg(code));
    if (res->req->res_cb) {
        res->req->res_cb((request_t*)res->req, code);
    }
}

void destory_response(response_p_t* res) {
    if (NULL == res) {
        return;
    }
    if (NULL != res->headers) {
        map_iterator_t it = map_begin(res->headers);
        map_iterator_t end = map_end(res->headers);
        for (; iterator_not_equal(it, end); it = iterator_next(it)) {
            pair_t* pt_pair = (pair_t*)iterator_get_pointer(it);
            string_t* key = *(string_t**)pair_first(pt_pair);
            string_t* value = *(string_t**)pair_second(pt_pair);
            string_destroy(key);
            string_destroy(value);
        }
        map_destroy(res->headers);
    }
    if (NULL != res->vesion)
        string_destroy(res->vesion);
    if (NULL != res->status_desc)
        string_destroy(res->status_desc);

    free(res);
}