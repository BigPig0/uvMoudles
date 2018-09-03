#include "uvHttp.h"
#include "typedef.h"


/** 检测连接超时的定时器 */
void uv_timer_cb(uv_timer_t* handle) {

}

void string_map_compare(const void* cpv_first, const void* cpv_second, void* pv_output) {
    *(bool_t*)pv_output = string_less(*(const string_t**)cpv_first, *(const string_t**)cpv_second) ? true : false;
}

/** 模块初始化 */
void agents_init(http_t* h) {
    h->agents = create_map(string_t*, agent_t*);
    map_init_ex(h->agents, string_map_compare);
    int ret = uv_timer_init(h->uv, &h->timeout_timer);
	h->timeout_timer.data = h;
    ret = uv_timer_start(&h->timeout_timer, uv_timer_cb, 10000, 10000);
}

/** 模块销毁 */
void agents_destory(http_t* h) {
	uv_timer_stop(&h->timeout_timer);
    map_destroy(h->agents);
}

/** 根据ip和端口获取一个agent */
agent_t* get_agent(http_t* h, string_t* addr) {
    map_iterator_t it_pos = map_find(h->agents, addr);
    if(iterator_equal(it_pos, map_end(h->agents))) {
        agent_t* new_agent = (agent_t*)malloc(sizeof(agent_t));
        memset(new_agent, 0, sizeof(agent_t));
        new_agent->handle = h;
        new_agent->req_list = create_list(request_t);
        list_init(new_agent->req_list);
        new_agent->sockets = create_set(socket_t*);
        set_init(new_agent->sockets);
        new_agent->free_sockets = create_set(socket_t*);
        set_init(new_agent->free_sockets);
        new_agent->keep_alive = true;
        pair_t* pt_pair = create_pair(string_t*, agent_t*);
        pair_init_elem(pt_pair, addr, new_agent);
        map_insert(h->agents, pt_pair);
        pair_destroy(pt_pair);
        return new_agent;
    } else {
        pair_t* pt_pair = (pair_t*)iterator_get_pointer(it_pos);
        agent_t* agent = *(agent_t**)pair_second(pt_pair);
        return agent;
    }
}

/** 在agent中创建一个请求 */
int agents_request(request_p_t* req) {
	//首先需要创建或获取一个agent
	string_t* addr = create_string();
	string_init(addr);
	string_connect(addr, req->str_addr);
	string_connect_char(addr, ':');
	string_connect(addr, req->str_port);
	agent_t* agent = get_agent(req->handle, addr);
	string_destroy(addr);

	//创建或者获取一个已经存在连接，如果连接数达到最大值则需要将请求放到队列中
    int sockets_num = set_size(agent->sockets);
    if (sockets_num >= agent->handle->conf.max_sockets){
        //将请求放到队列中
		if (agent->requests == NULL) {
			agent->requests = create_list(void*);
			list_init_elem(agent->requests, 1, req);
		} else {
			list_push_back(agent->requests, req);
		}
    } else if (agent->free_sockets == NULL || set_empty(agent->free_sockets)) {
        //新建一个连接来处理请求
		socket_t* socket = (socket_t*)malloc(sizeof(socket_t));
    } else {
        //从空闲请求中取出一个来处理请求
    }
}