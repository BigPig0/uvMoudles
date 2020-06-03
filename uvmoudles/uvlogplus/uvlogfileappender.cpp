#include "uvlogprivate.h"
#include "uv.h"
#include "uvlogutil.h"
#include "utilc.h"
#include "TimeFormat.h"

namespace uvLogPlus {

static void _write_task_fs_cb(uv_fs_t* req) {
    LogMsgReq *msg_req = (LogMsgReq*)req->data;
    FileAppender* apd = (FileAppender*)msg_req->appender;
    SAFE_FREE(msg_req->buff);
    delete msg_req;
    uv_fs_req_cleanup(req);
    delete req;

    apd->WriteCB();
}

FileAppender::FileAppender()
    : append(false)
    , opening(false)
    , opened(false)
    , writing(false)
{

}

FileAppender::~FileAppender() {

}

void FileAppender::Init(uv_loop_t *uv) {
    uv_loop = uv;
    if(!opened && !opening){
        opening = true;
        //ȷ���ϼ�Ŀ¼�ѱ�����
        if(file_sys_check_path(file_name.c_str())){
            printf("create log file dir failed");
            opening = false;
            opened = true;
            return;
        }
        uv_fs_t req;
        int ret = 0;
        int tag = UV_FS_O_RDWR | UV_FS_O_CREAT | UV_FS_O_APPEND;
        if(!append)
            tag |= UV_FS_O_TRUNC;
        ret = uv_fs_open(NULL, &req, file_name.c_str(), tag, 666, NULL);
        if(ret < 0) {
            printf("fs open %s failed: %s\r\n", file_name.c_str(), strerror(ret));
        } else {
            file_handle = req.result;
            opened = true;
        }
        opening = false;
    } 
}

void FileAppender::Write() {
    if(!opened || writing) {
        return;
    }

    //�Ӷ�����ȡ��һ������
    std::shared_ptr<LogMsg> item;
    bool found = msg_queue.try_dequeue(item);
    if(!found)
        return;
    writing = true;

    //������־����
    LogMsgReq *msg_req = new LogMsgReq;
    msg_req->appender = this;
    msg_req->item = item;
    msg_req->buff = (char *)calloc(1, 100);
    char *pos = msg_req->buff;
    //�߳�ID
    char *szThread = pos;
    sprintf(szThread, "[%08d]", item->tid);
    pos += 20;
    //ʱ��
    char *szTime = pos;
    CTimeFormat::printTime(item->msg_time, "%Y%m%d%H%M%S ", szTime);
    pos += 32;
    //�ȼ�
    char *szLevel = levelNote[(int)item->level];
    //�ļ���
    char *szFile = (char*)item->file_name;
    int nNameLen = strlen(item->file_name);
    if(nNameLen > 50) {
        szFile = (char*)item->file_name + (nNameLen - 50);
        nNameLen = 50;
    }
    //�к�
    char *szLine = pos;
    sprintf(szLine, ":%08d\t", item->line);
    pos += 15;
    //������
    int nFuncLen = strlen(item->func_name);
    if(nFuncLen > 50)
        nFuncLen = 50;

    uv_fs_t * req = new uv_fs_t;
    req->data = msg_req;
    uv_buf_t buff[10];
    buff[0] = uv_buf_init(szThread, 10);
    buff[1] = uv_buf_init(szTime, strlen(szTime));
    buff[2] = uv_buf_init(szLevel, strlen(szLevel));
    buff[3] = uv_buf_init((char*)"\t", 1);
    buff[4] = uv_buf_init(szFile, nNameLen);
    buff[5] = uv_buf_init(szLine, 10);
    buff[6] = uv_buf_init((char*)item->func_name, nFuncLen);
    buff[7] = buff[3];
    buff[8] = uv_buf_init((char*)item->msg.c_str(), item->msg.size());
    buff[9]= uv_buf_init((char*)LINE_SEPARATOR, strlen(LINE_SEPARATOR));
    uv_fs_write(uv_loop, req, file_handle, buff, 10, 0, _write_task_fs_cb);
}

void FileAppender::WriteCB(){
    //д��һ����־
    writing = false;
    Write();
}

}