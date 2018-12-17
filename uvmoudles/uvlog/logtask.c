#include "st.h"
#include "utilc.h"
#include "cstl_easy.h"

task_log_msg_t* create_task_log_msg() {
    SAFE_MALLOC(task_log_msg_t, ret);
    ret->name = create_string();
    ret->msg = create_string();

    return ret;
}

void destory_task_log_msg(task_log_msg_t *task) {
    string_destroy(task->name);
    string_destroy(task->msg);
    free(task);
}

task_fifo_queue_t* create_task_fifo_queue() {
    SAFE_MALLOC(task_fifo_queue_t, ret);
    ret->queue[0].queue = create_map(int, void*);
    map_init(ret->queue[0].queue);
    uv_rwlock_init(&ret->queue[0].lock);
    ret->queue[1].queue = create_map(int, void*);
    map_init(ret->queue[1].queue);
    uv_rwlock_init(&ret->queue[1].lock);

    return ret;
}

void destory_task_fifo_queue(task_fifo_queue_t *queue) {
    MAP_DESTORY(queue->queue[0].queue, int, task_log_msg_t*, not_free_int, destory_task_log_msg);
    MAP_DESTORY(queue->queue[1].queue, int, task_log_msg_t*, not_free_int, destory_task_log_msg);
}

void add_task(task_fifo_queue_t *queue, task_log_msg_t* task) {
    map_t *insert_queue;
    int task_id;

    if (queue->last_queue_id != queue->queue_id) {
        queue->last_queue_id = queue->queue_id;
        queue->queue[queue->queue_id].task_id = 0;
    }

    insert_queue = queue->queue[queue->queue_id].queue;
    task_id = queue->queue[queue->queue_id].task_id++;

    uv_rwlock_rdlock(&queue->queue[queue->queue_id].lock);
    //MAP_INSERT(insert_queue, int, task_id, void*, task);
    {
        pair_t* pt_pair = create_pair(int, void*);
        pair_init_elem(pt_pair, task_id, task);
        map_insert(insert_queue, pt_pair);
        pair_destroy(pt_pair);
    }
    uv_rwlock_rdunlock(&queue->queue[queue->queue_id].lock);
}

void get_task(uv_log_handle_t* h, task_fifo_queue_t *queue, task_func cb) {
    task_queue_t *queue_get = &queue->queue[queue->queue_id];
    queue->queue_id = queue->queue_id?0:1;

    uv_rwlock_wrlock(&queue_get->lock);
    MAP_FOR_BEGIN(queue_get->queue, int, key, task_log_msg_t*, value)
        cb(h, value);
        free(value);
    MAP_FOR_END
    map_clear(queue_get->queue);
    uv_rwlock_wrunlock(&queue_get->lock);
}