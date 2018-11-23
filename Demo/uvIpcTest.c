#include <stdio.h>
#include <tchar.h>
#include "uvIpc.h"
#include "uv.h"
#include "util_api.h"

uv_loop_t *loop;
uv_process_t child_req;
uv_process_options_t options;
uv_process_t child_req2;
uv_process_options_t options2;
uv_ipc_handle_t *ipc;
uv_timer_t sender_timer;

struct IPC_TEST{
    uv_ipc_handle_t *ipc;
    char *tag;
};
struct IPC_TEST user_data;

void on_exit(uv_process_t* req, int64_t exit_status, int term_signal) {
    printf("on exit %d\n", req->pid);
    uv_close((uv_handle_t*)req, NULL);
}

void run_child_1(char* path) {
    int ret;
    char* args[3];
    uv_stdio_container_t child_stdio[3];

    args[0] = path;
    args[1] = "1";
    args[2] = NULL;

    child_stdio[0].flags = UV_IGNORE;
    child_stdio[1].flags = UV_INHERIT_FD;
    child_stdio[1].data.fd = 1;
    child_stdio[2].flags = UV_INHERIT_FD;
    child_stdio[2].data.fd = 2;
    options.stdio_count = 3;
    options.stdio = child_stdio;

    options.exit_cb = on_exit;
    options.file = path;
    options.args = args;

    ret = uv_spawn(loop, &child_req, &options);
    if (ret < 0) {
        fprintf(stderr, "%s\n", uv_strerror(ret));
        return;
    }

    printf("run %d: %s \n",child_req.pid, path);
}

void run_child_2(char* path) {
    int ret;
    char* args[3];
    uv_stdio_container_t child_stdio[3];

    args[0] = path;
    args[1] = "2";
    args[2] = NULL;

    child_stdio[0].flags = UV_IGNORE;
    child_stdio[1].flags = UV_INHERIT_FD;
    child_stdio[1].data.fd = 1;
    child_stdio[2].flags = UV_INHERIT_FD;
    child_stdio[2].data.fd = 2;
    options2.stdio_count = 3;
    options2.stdio = child_stdio;

    options2.exit_cb = on_exit;
    options2.file = path;
    options2.args = args;

    ret = uv_spawn(loop, &child_req2, &options2);
    if (ret < 0) {
        fprintf(stderr, "%s\n", uv_strerror(ret));
        return;
    }

    printf("run %d: %s \n",child_req2.pid, path);
}

void on_uv_ipc_recv_cb(uv_ipc_handle_t* h, void* user, char* name, char* msg, char* data, int len) {
    char buff[100]={0};
    sprintf(buff, "recv: sender:%s msg:%s, data:%s\n", name, msg, data);
}

int send_num = 0;
void timer_cb(uv_timer_t* handle)
{
    struct IPC_TEST *test = (struct IPC_TEST *)handle;
    if(!strcmp(test->tag, "1")) {
        char buff[100]={0};
        sprintf(buff, "send: this is from 1 to 2 %d", send_num++);
        uv_ipc_send(ipc, "test2", "cmd", buff, strlen(buff));
    } else {
        char buff[100]={0};
        sprintf(buff, "send: this is from 2 to 1 %d", send_num++);
        uv_ipc_send(ipc, "sender1", "haha", buff, strlen(buff));
    }
}

int main(int argc, char* argv[])
{
    loop = uv_default_loop();

    if(argc == 1) {
        uv_ipc_server(&ipc, "testIPC", loop);
        run_child_1(argv[0]);
        run_child_2(argv[0]);
    } else {
        char* ipc_name;
        while (1)
        {
            sleep(1000);
        }
        if(!strcmp(argv[1], "1")) {
            ipc_name = "sender1";
            user_data.tag = "1";
        } else {
            ipc_name = "test2";
            user_data.tag = "2";
        }
        uv_ipc_client(&ipc, "testIPC", loop, ipc_name, on_uv_ipc_recv_cb, NULL);
        user_data.ipc = ipc;

        uv_timer_init(loop, &sender_timer);
        sender_timer.data = (void*)&user_data;
        uv_timer_start(&sender_timer, timer_cb,3000, 3000);
    }


    while(1) {
        uv_run(loop, UV_RUN_DEFAULT);
        sleep(1000);
    }
    
    sleep(INFINITE);
    return 0;
}