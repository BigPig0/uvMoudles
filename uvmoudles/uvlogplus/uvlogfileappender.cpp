#include "uvlogprivate.h"
#include "uv.h"

namespace uvLogPlus {

static void _write_task_fs_cb(uv_fs_t* req) {
    LogMsgReq *msg_req = (LogMsgReq*)req->data;
    FileAppender* apd = (FileAppender*)msg_req->appender;
    delete msg_req;
    delete req;

    //写下一条日志
    apd->Write();
}

FileAppender::FileAppender()
    : opened(false)
    , opening(false)
    , append(false)
{

}

FileAppender::~FileAppender() {

}

void FileAppender::Init(uv_loop_t *uv) {
    uv_loop = uv;
    if(!opened && !opening){
        uv_fs_t *req = new uv_fs_t;
        int ret = 0;
        int tag = O_RDWR | O_CREAT;
        if(append)
            tag |= O_APPEND;
        ret = uv_fs_open(NULL, req, file_name.c_str(), tag, 0, NULL);
        if(ret < 0) {
            printf("fs open %s failed: %s\r\n", file_name.c_str(), strerror(ret));
        }
        file_handle = req->result;
        delete req;
    } 
}

void FileAppender::Write() {
    if(opened) {
        std::shared_ptr<LogMsg> item;
        bool found = msg_queue.try_dequeue(item);
        if(!found)
            return;

        LogMsgReq *msg_req = new LogMsgReq;
        msg_req->appender = this;
        msg_req->item = item;
        msg_req->buff = uv_buf_init((char*)item->msg.c_str(), item->msg.size());

        uv_fs_t * req = new uv_fs_t;
        req->data = msg_req;
        uv_fs_write(uv_loop, req, file_handle, &msg_req->buff, 1, 0, _write_task_fs_cb);
    }
}

}