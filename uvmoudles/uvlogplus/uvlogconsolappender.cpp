#include "uvlogprivate.h"
#include "uv.h"

namespace uvLogPlus {

static void _write_task_cb(uv_write_t* req, int status) {
    LogMsgReq *msg_req = (LogMsgReq*)req->data;
    ConsolAppender* apd = (ConsolAppender*)msg_req->appender;
    delete msg_req;
    delete req;

    //写下一条日志
    apd->Write();
}

ConsolAppender::ConsolAppender()
    : opened(false)
    , opening(false)
    , target(ConsolTarget::SYSTEM_OUT)
{
    type = AppenderType::consol;
}

ConsolAppender::~ConsolAppender() {

}

void ConsolAppender::Init(uv_loop_t *uv) {
    uv_loop = uv;
    if(!opened && !opening){
        int ret = 0;
        uv_file fd = 1; //stdout
        if(target == ConsolTarget::SYSTEM_ERR)
            fd = 2;
        opening = true;
        ret = uv_tty_init(uv, &tty_handle, fd, 0);
        if(ret < 0) {
            printf("tty init failed: %s\r\n", strerror(ret));
        }
        opened = true;
    }
}

void ConsolAppender::Write() {
    if(opened){
        std::shared_ptr<LogMsg> item;
        bool found = msg_queue.try_dequeue(item);
        if(!found)
            return;
        
        LogMsgReq *msg_req = new LogMsgReq;
        msg_req->appender = this;
        msg_req->item = item;
        msg_req->buff = uv_buf_init((char*)item->msg.c_str(), item->msg.size());

        uv_write_t *req = new uv_write_t;
        req->data = msg_req;
        uv_write(req, (uv_stream_t*)&tty_handle, &msg_req->buff, 1, _write_task_cb);
    }
}

}