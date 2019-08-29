#include "public.h"
#include "private.h"




/** 检测连接超时的定时器 */
static void timer_cb(uv_timer_t* handle) {
    map_iterator_t it_pos, it_end;
    pair_t* pt_pair;
    agent_t* agent;
    set_iterator_t s_pos, s_end;
    socket_t* s;
    time_t now;
    uv_node_t* h = (uv_node_t*)handle->data;
    if(!h || !h->agents)
        return;

    uv_mutex_lock(&h->uv_mutex_h);
    now = time(NULL);
    it_pos = map_begin(h->agents);
    it_end = map_end(h->agents);
    for(;iterator_not_equal(it_pos, it_end);it_pos = iterator_next(it_pos)) {
        pt_pair = (pair_t*)iterator_get_pointer(it_pos);
        agent = *(agent_t**)pair_second(pt_pair);
        s_pos = set_begin(agent->sockets);
        s_end = set_end(agent->sockets);
        for(;iterator_not_equal(s_pos, s_end);s_pos = iterator_next(s_pos)) {
            s = *(socket_t**)iterator_get_pointer(s_pos);
            if(s->req->req_time > 0) {
                if(s->req->req_step == request_step_dns &&
                    difftime(now, s->req->req_time) > h->conf.request_timeout_secs){
                        printf("request time out\n");
                } else if(s->req->req_step == request_step_send &&
                    difftime(now, s->req->req_time) > h->conf.response_timeout_secs){
                        printf("response time out\n");
                }
            }
        }
        ;
    }
    uv_mutex_unlock(&h->uv_mutex_h);
}

void string_map_compare(const void* cpv_first, const void* cpv_second, void* pv_output) {
    *(bool_t*)pv_output = string_less(*(const string_t**)cpv_first, *(const string_t**)cpv_second) /*? true : false*/;
    //const string_t* cpstr_first = *(const string_t**)cpv_first;
    //const string_t* cpstr_second = *(const string_t**)cpv_second;
    //bool_t ret = string_less(cpstr_first, cpstr_second);
    //*(bool_t*)pv_output = ret;
}

/** 模块初始化 */
void agents_init(uv_node_t* h) {
    int ret;
    h->agents = create_map(void*, void*); //map<string_t*, agent_t*>
    map_init_ex(h->agents, string_map_compare);
    ret = uv_timer_init(h->uv, &h->timeout_timer);
    h->timeout_timer.data = h;
    ret = uv_timer_start(&h->timeout_timer, timer_cb, 1000, 1000);
}

/** 模块销毁 */
void agents_destory(uv_node_t* h) {
    uv_timer_stop(&h->timeout_timer);
    map_destroy(h->agents);
}

/** 根据ip和端口获取一个agent */
agent_t* get_agent(uv_node_t* h, string_t* addr) {
    agent_t* ret;
    map_iterator_t it_pos;
    uv_mutex_lock(&h->uv_mutex_h);
    printf("agents num:%d\r\n", map_size(h->agents));
    it_pos = map_find(h->agents, addr);
    if(iterator_equal(it_pos, map_end(h->agents))) {
        pair_t* pt_pair;
        agent_t* new_agent = (agent_t*)malloc(sizeof(agent_t));
        printf("agent[%s] not exist\r\n", string_c_str(addr));
        memset(new_agent, 0, sizeof(agent_t));
        new_agent->handle = h;
        new_agent->req_list = create_list(void*);   //list<request_t*>
        list_init(new_agent->req_list);
        new_agent->sockets = create_set(void*);     //set<socket_t*>
        set_init(new_agent->sockets);
        new_agent->free_sockets = create_set(void*); //set<socket_t*>
        set_init(new_agent->free_sockets);
        new_agent->keep_alive = true;
        pt_pair = create_pair(void*, void*); //pair<string_t*, agent_t*>
        pair_init_elem(pt_pair, addr, new_agent);
        map_insert(h->agents, pt_pair);
        pair_destroy(pt_pair);
        uv_mutex_init(&new_agent->uv_mutex_h);
        ret = new_agent;
    } else {
        pair_t* pt_pair = (pair_t*)iterator_get_pointer(it_pos);
        agent_t* agent = *(agent_t**)pair_second(pt_pair);
        printf("agent[%s] is exist\r\n", string_c_str(addr));
        string_destroy(addr);
        ret = agent;
    }
    uv_mutex_unlock(&h->uv_mutex_h);
    return ret;
}

/** 在agent中发送一个请求 */
int agent_request(request_p_t* req) {
    agent_t* agent = NULL;
    bool_t can_run = false;
    socket_t* socket = NULL;
    //首先需要创建或获取一个agent
    string_t* addr = create_string();   //地址在map释放时释放
    string_init(addr);
    string_connect(addr, req->str_addr);
    string_connect_char(addr, ':');
    string_connect(addr, req->str_port);
    agent = get_agent(req->handle, addr);

    //创建或者获取一个已经存在连接，如果连接数达到最大值则需要将请求放到队列中
    uv_mutex_lock(&agent->uv_mutex_h);
    do {
        int sockets_num = set_size(agent->sockets);
        printf("sockets num:%d\r\n", sockets_num);
        if (sockets_num >= agent->handle->conf.max_sockets){
            //将请求放到队列中
            printf("put reques to list\r\n");
            if (agent->requests == NULL) {
                agent->requests = create_list(void*);  //list<request_t*>
                list_init_elem(agent->requests, 1, req);
            } else {
                list_push_back(agent->requests, req);
            }
            break;
        } else if (agent->free_sockets == NULL || set_empty(agent->free_sockets)) {
            //新建一个连接来处理请求
            printf("creat a new request\r\n");
            socket = create_socket(agent);
            socket->req = req;
            set_insert(agent->sockets, socket);
            can_run = true;
        } else {
            //从空闲请求中取出一个来处理请求
            set_iterator_t it_socket;
            printf("get a socket from free list\n");
            it_socket = set_begin(agent->free_sockets);
            socket = *(socket_t**)iterator_get_pointer(it_socket);
            set_erase(agent->free_sockets, socket);
            socket->req = req;
            set_insert(agent->sockets, socket);
            can_run = true;
        }
    } while (0);
    uv_mutex_lock(&agent->uv_mutex_h);

    if(can_run)
        socket_run(socket);
    return uv_http_ok;
}

void agent_request_finish(bool_t ok, socket_t* socket) {
    agent_t* agent = (agent_t*)socket->agent;
    bool_t can_run = false;
    socket_t* next_run;
    uv_mutex_lock(&agent->uv_mutex_h);
    if (ok) {
        int keep_alive = socket->req->res->keep_alive;
        destory_request(socket->req);
        socket->req = NULL;
        if (agent->requests != NULL && list_size(agent->requests) > 0) {
            //请求接收应答完成，且存在未执行的请求，继续执行请求
            request_p_t* req = *(request_p_t**)list_front(agent->requests);
            list_pop_front(agent->requests);
            socket->req = req;
            can_run = true;
            next_run = socket;
            printf("1 finish a request, and run next. request num:%d,sockets num:%d,free_socket_num:%d\r\n"
                , list_size(agent->requests), set_size(agent->sockets), set_size(agent->free_sockets));
        } else {
            //请求接收应答完成, 无未执行的请求，将socket移动到空闲队列
            int e_num = set_erase(agent->sockets, socket);
            int free_sockets_num = set_size(agent->free_sockets);
            if (free_sockets_num >= agent->handle->conf.max_free_sockets) {
                //空闲队列已满，销毁socket
                destory_socket(socket);
            } else {
                set_insert(agent->free_sockets, socket);
            }
            printf("2 finish a request, and move to free list. sockets num:%d,free_socket_num:%d\r\n"
                , set_size(agent->sockets), set_size(agent->free_sockets));
        }
    } else {
        //请求执行失败，销毁socket
        set_iterator_t it;
        destory_request(socket->req);
        socket->req = NULL;
        it = set_find(agent->sockets, socket);
        if (iterator_not_equal(it, set_end(agent->sockets))) {
            int e_num = set_erase(agent->sockets, socket);
            destory_socket(socket);
            if (agent->requests != NULL && list_size(agent->requests) > 0) {
                //存在未执行的请求，创建新socket执行请求
                request_p_t* req = (request_p_t*)list_front(agent->requests);
                list_pop_front(agent->requests);
                next_run = create_socket(agent);
                next_run->req = req;
                set_insert(agent->sockets, next_run);
                can_run = true;
                printf("3 failed a request, and run next. request num:%d,sockets num:%d,free_socket_num:%d\r\n"
                    , list_size(agent->requests), set_size(agent->sockets), set_size(agent->free_sockets));
            } else {
                printf("4 failed a request. sockets num:%d,free_socket_num:%d\r\n"
                    , set_size(agent->sockets), set_size(agent->free_sockets));
            }
        } else {
            it = set_find(agent->free_sockets, socket);
            if (iterator_not_equal(it, set_end(agent->free_sockets))) {
                int e_num = set_erase(agent->free_sockets, socket);
                destory_socket(socket);
            }
            printf("5 free_socket list disconnect. sockets num:%d,free_socket_num:%d\r\n"
                , set_size(agent->sockets), set_size(agent->free_sockets));
        }
    }
    uv_mutex_unlock(&agent->uv_mutex_h);

    if(can_run)
        socket_run(socket);
}

void agent_socket_disconnect(socket_t* socket) {
    set_iterator_t it;
    agent_t* agent = (agent_t*)socket->agent;
    uv_mutex_lock(&agent->uv_mutex_h);
    it = set_find(agent->sockets, socket);
    if (iterator_not_equal(it, set_begin(agent->sockets))) {
    } else {
        it = set_find(agent->free_sockets, socket);
        if (iterator_not_equal(it, set_begin(agent->free_sockets))) {
        }
    }
    uv_mutex_unlock(&agent->uv_mutex_h);
}