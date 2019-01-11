#include "uvnode.h"
#include "private.h"
#include "uv.h"
#include "cstl_easy.h"

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
    uv_node_t* h = (uv_node_t*)arg;
    while (h->is_run) {
        uv_run(h->uv, UV_RUN_DEFAULT);
        Sleep(1000);
    }
    uv_mutex_destroy(&h->uv_mutex_h);
    list_destroy(h->async_event);
    uv_loop_close(h->uv);
    free(h->uv);
    free(h);
}

static void on_uv_async(uv_async_t* handle) {
    uv_node_t* h = (uv_node_t*)handle->data;
    uv_mutex_lock(&h->uv_mutex_h);
    LIST_FOR_BEGIN(h->async_event, uv_async_event_params_t*, ep) {
        if(ep->event == ASYNC_EVENT_THREAD_ID) {
            h->loop_tid = uv_thread_self();
        } else if(ep->event == ASYNC_EVENT_DNS_LOOKUP) {
            dns_lookup_async(ep->params);
        } else if(ep->event == ASYNC_EVENT_TCP_SERVER_INIT) {
            net_create_server_async(ep->params);
        }
    }LIST_FOR_END;
    LIST_DESTORY(h->async_event, uv_async_event_params_t*, free);
    uv_mutex_unlock(&h->uv_mutex_h);
}

uv_node_t* uv_node_create( void* uv) {
    uv_node_t* h = (uv_node_t*)malloc(sizeof(uv_node_t));
    int ret;

    /** event loop */
	if (uv != NULL) {
		h->uv = (uv_loop_t*)uv;
		h->inner_uv = false;
	} else {
		h->uv = (uv_loop_t*)malloc(sizeof(uv_loop_t));
		ret = uv_loop_init(h->uv);
        if(ret < 0) {
            printf("uv loop init failed: %s\n", uv_strerror(ret));
            free(h->uv);
            free(h);
            return NULL;
        }
		h->inner_uv = true;
    }

    /** async */
    h->uv_async_h.data = (void*)h;
    uv_async_init(h->uv, &h->uv_async_h, on_uv_async);
    h->async_event = create_list(void*);
    list_init(h->async_event);
    uv_mutex_init(&h->uv_mutex_h);

    /** run loop and get thread id */
    h->is_run = true;
    if(h->inner_uv) {
        ret = uv_thread_create(&h->loop_tid, run_loop_thread, h);
        if(ret < 0) {
            printf("uv thread creat failed: %s\n", uv_strerror(ret));
            uv_mutex_destroy(&h->uv_mutex_h);
            list_destroy(h->async_event);
            free(h->uv);
            free(h);
            return NULL;
        }
    } else {
        SAFE_MALLOC(uv_async_event_params_t, e);
        e->event = ASYNC_EVENT_THREAD_ID;
        uv_mutex_lock(&h->uv_mutex_h);
        list_push_back(h->async_event, e);
        uv_mutex_unlock(&h->uv_mutex_h);
        uv_async_send(&h->uv_async_h);
    }

	return h;
}

void uv_node_close(uv_node_t* h) {
	if (h->inner_uv == true) {
		h->is_run = false;
		uv_stop(h->uv);
	}
    else {
        uv_mutex_destroy(&h->uv_mutex_h);
        list_destroy(h->async_event);
		free(h);
	}
}

void send_async_event(uv_node_t* h, uv_async_event_t type, void *p) {
    SAFE_MALLOC(uv_async_event_params_t, e);
    e->event = type;
    e->params = p;
    uv_mutex_lock(&h->uv_mutex_h);
    list_push_back(h->async_event, e);
    uv_mutex_unlock(&h->uv_mutex_h);
    uv_async_send(&h->uv_async_h);
}
