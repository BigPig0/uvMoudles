#include "util.h"
#include "utilc.h"
#include "easylog.h"
#include "uv.h"
#include "uvnetplus.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <thread>
using namespace std;
using namespace uvNetPlus;
using namespace uvNetPlus::Ftp;

CNet* net = NULL;

//获取文件列表
static void OnFtpList(CFtpConnect *req, list<CFtpFile> files) {
    for(auto f:files) {
        Log::warning("%s %s %s", f.permission.c_str(), f.date.c_str(), f.name.c_str());
    }
}

//切换目录
static void OnChangeWorkingDirectory(CFtpConnect *req) {
    Log::debug("change working directory success");
    req->List(OnFtpList);
}

//获取文件列表
static void OnNameList(CFtpConnect *req, std::list<std::string> names) {
    for(auto str:names) {
        Log::warning(str.c_str());
    }
}

//上传文件
static void OnUpload(CFtpConnect *req) {
    Log::debug("upload success");
}

//下载文件
static void OnDownload(CFtpConnect *req, char* data, uint32_t size) {
    FILE *f = fopen("d:\\test2.jpg", "wb+");
    fwrite(data, 1, size, f);
    fclose(f);
}

//创建文件夹
static void OnMakeDirectory(CFtpConnect *req) {
    Log::debug("make directory success");
}

//删除文件夹
static void OnRemoveDirectory(CFtpConnect *req) {
    Log::debug("remove directory success");
}

//删除文件
static void OnDeleteFile(CFtpConnect *req) {
    Log::debug("delete file sucess");
}

//异常分支回调
static void OnFtpCb(CFtpConnect *req, CFtpMsg *msg) {
    Log::error("%s", msg->replyStr.c_str());
}

// 登陆成功
static void OnFtpLogin(CFtpConnect *req) {
    req->ChangeWorkingDirectory("/var/ftp/vioupload", OnChangeWorkingDirectory);
    /*
    req->NameList(OnNameList);
    */
    /*
    req->List(OnFtpList);
    */
    /*
    char* data = (char*)calloc(1, 1024*1024*10); //10M空间
    FILE *f = fopen("d:\\test.jpg", "rb+");
    int len = fread(data, 1, 1024*1024*10, f);
    req->Upload("testxxx.jpg", data, len, OnUpload);
    */
    /*
    //req->Download("testxxx.jpg", OnDownload);
    req->Download("notexist.jpg", OnDownload);
    */
    /*
    req->MakeDirectory("123/456", OnMakeDirectory);
    */
    /*
    req->RmDirectory("123", OnRemoveDirectory);
    */
    /*
    req->DelFile("testxxx.jpg", OnDeleteFile);
    */
}

int main()
{
    Log::open(Log::Print::both, uvLogPlus::Level::Debug, "./log/unNetPlusFtp/log.txt");
    net = CNet::Create();
    CFtpClient* ftpClient = CFtpClient::Create(net);
    ftpClient->Connect("192.168.2.111", 21, "ftp", "123", OnFtpLogin, OnFtpCb);

    sleep(INFINITE);
}