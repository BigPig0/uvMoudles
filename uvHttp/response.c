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

bool response_recive(response_p_t* res, char* data, int len) {
    printf("response_recive,%s len:%d\r\n", res->req->url, len);

        size_t i = 0;
        string_t* str_tmp = create_string();
        string_t* str_tmp2 = create_string();
        string_init(str_tmp);
        string_init(str_tmp2);
        for(; i < len; ++i)
        {
            if(response_step_notbegin == res->parsed_headers)
            {
                if(data[i]=='h'&&data[i+1]=='t'&&data[i+2]=='t'&&data[i+3]=='p') {
                    res->parsed_headers = response_step_protocol;
                }
            } else if(response_step_protocol == res->parsed_headers) {
                if(data[i] == ' ') {
                    res->parsed_headers = response_step_stause_begin;
                }
            } else if(response_step_stause_begin == res->parsed_headers) {
                if(data[i] == ' ') {
                    res->status = atoi(string_c_str(str_tmp));
                    string_clear(str_tmp);
                    res->parsed_headers = response_step_status_ok;
                } else {
                    string_push_back(str_tmp, data[i]);
                }
            } else if(response_step_status_ok == res->parsed_headers) {
                if (data[i] == '\r' && data[i+1] == '\n') {
                    string_clear(str_tmp);
                    res->parsed_headers = response_step_status_desc;
                } else {
                    string_push_back(str_tmp, data[i]);
                }
            } else if(response_step_status_desc == res->parsed_headers) {
                if(data[i] == ':') {
                    res->parsed_headers = response_step_header_key;
                } else {
                    string_push_back(str_tmp, data[i]);
                }
            } else if(response_step_header_key == res->parsed_headers) {
                if (data[i] == '\r' && data[i+1] == '\n') {
                    string_t* str_key = create_string();
                    string_t* str_value = create_string();
                    pair_t* pt_pair = create_pair(void*, void*);
                    string_init_copy(str_key, str_tmp);
                    string_init_copy(str_value, str_tmp2);
                    pair_init_elem(pt_pair, str_key, str_value);
                    map_insert(res->headers, pt_pair);
                    pair_destroy(pt_pair);
                    string_clear(str_tmp);
                    string_clear(str_tmp2);
                } else {
                    if(data[i] != ' ') string_push_back(str_tmp2, data[i]);
                }
            } else if(response_step_header_value == res->parsed_headers) {

            } else if(response_step_header_end == res->parsed_headers) {

            }
            if( 1 != mf_write(res->header_buff, &data[i], 1))
            {
                /* 超出 HTTP Request 头的长度限制了 */
                /* 凡是从客户端读取的数据都是不安全的数据,所以设置了一个最大值 */
                return false;
            }

            int nHeadSize = res->header_buff->_fileSize;
            if( nHeadSize<4 )
                continue;

            char* pHeader = (char*)mf_buffer(res->header_buff);
            if (   pHeader[nHeadSize-4] == '\r'
                && pHeader[nHeadSize-3] == '\n'
                && pHeader[nHeadSize-2] == '\r'
                && pHeader[nHeadSize-1] == '\n')
            {
                // 已经接收到一个完整的请求头
                do
                {
                    // 查找Content-Length
                    const char* pszStart = strstr(pHeader, "Content-Length:");
                    if(pszStart == NULL || (pszStart != pHeader && *(pszStart - 1) != '\n'))
                        break;
                    pszStart += 15;

                    // 找到字段结束
                    const char* pszEnd = strstr(pszStart, "\r\n");
                    if(pszEnd == NULL)
                        break;

                    //长度值
                    string strValue;
                    strValue.assign(pszStart, pszEnd - pszStart);
                    m_nContentLength = atoi(strValue.c_str());
                }while (0);

                m_bHeaderRecved = true;
                Log::debug("请求头: \r\n%s",pHeader);

                // 单独接收content数据
                if(m_nContentLength >= MAX_POST_DATA)
                {
                    /* 检查 content-length 长度是否超出限制 */
                    return 0;
                }

                if(++i < len)
                {
                    /* 还有数据 */
                    i += push(data + i, len - i);
                }

                break;
            }
        }
}

void response_error(response_p_t* res, int code) {
    printf("response_error: %s", uvhttp_err_msg(code));
    if(res->req->res_cb) {
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