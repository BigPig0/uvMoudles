#include "uvHttp.h"
#include "private_def.h"

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

void run_loop_thread(void* arg)
{
    http_t* h = (http_t*)arg;
    while (h->is_run) {
        uv_run(h->uv, UV_RUN_DEFAULT);
        Sleep(2000);
    }
    uv_loop_close(h->uv);
    free(h);
}

http_t* uvHttp(config_t cof, void* uv) {
    http_t* h = (http_t*)malloc(sizeof(http_t));
    h->conf = cof;
	uv_mutex_init(&h->uv_mutex_h);
	h->is_run = true;
	if (uv != NULL) {
		h->uv = (uv_loop_t*)uv;
		h->inner_uv = false;
	} else {
        int ret;
		h->uv = (uv_loop_t*)malloc(sizeof(uv_loop_t));
		ret = uv_loop_init(h->uv);
        if(ret < 0) {
            printf("uv loop init failed: %s\n", uv_strerror(ret));
            free(h->uv);
            free(h);
            return NULL;
        }
		h->inner_uv = true;
        uv_thread_t tid;
        ret = uv_thread_create(&tid, run_loop_thread, h);
        if(ret < 0) {
            printf("uv thread creat failed: %s\n", uv_strerror(ret));
            free(h->uv);
            free(h);
            return NULL;
        }
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
