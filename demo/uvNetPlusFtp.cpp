
#include "uvnetplus.h"
#include "Log.h"
#include "uv.h"
#include "utilc_api.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <thread>
using namespace std;
using namespace uvNetPlus;
using namespace uvNetPlus::Ftp;

CNet* net = NULL;

//获取文件列表
static void OnFtpList(CFtpRequest *req, list<CFtpFile> files) {
    for(auto f:files) {
        Log::warning("%s %s %s", f.permission.c_str(), f.date.c_str(), f.name.c_str());
    }
}

//获取文件列表
static void OnNameList(CFtpRequest *req, std::list<std::string> names) {
    for(auto str:names) {
        Log::warning(str.c_str());
    }
}

//上传文件
static void OnUpload(CFtpRequest *req) {
    Log::debug("upload success");
}

//下载文件
static void OnDownload(CFtpRequest *req, char* data, uint32_t size) {
    FILE *f = fopen("d:\\test2.jpg", "wb+");
    fwrite(data, 1, size, f);
    fclose(f);
}

//异常分支回调
static void OnFtpCb(CFtpRequest *req, CFtpMsg *msg) {
    Log::error("%s", msg->replyStr.c_str());
}

// 登陆成功
static void OnFtpLogin(CFtpRequest *req) {
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
    //req->Download("testxxx.jpg", OnDownload);
    req->Download("notexist.jpg", OnDownload);
}

int main()
{
    Log::open(Log::Print::both, Log::Level::debug, "./log/unNetPlusFtp.txt");
    net = CNet::Create();
    CFtpClient* ftpClient = CFtpClient::Create(net);
    ftpClient->Request("192.168.2.111", 21, "ftp", "123", OnFtpLogin, OnFtpCb);

    sleep(INFINITE);
}