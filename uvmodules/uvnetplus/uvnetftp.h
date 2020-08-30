/**
 * FTP（FILE TRANSFER PROTOCOL）：文件传输协议
 * PI（protocol interpreter）：协议解析器 用户和服务器用其来解析协议，它们的具体实现分别称为用户 PI （USER-PI）和服务器PI（SERVER-PI）
 * 服务器PI（server-PI）：服务器 PI 在 L 端口“监听”用户协议解析器的连接请求并建立控制连接。它从用户 PI接收标准的 FTP 命令，发送响应，并管理服务器 DTP
 * 服务器DTP（server-DTP）：数据传输过程，在通常的“主动”状态下是用“监听”的数据端口建立数据连接。它建立传输和存储参数，并在服务器端 PI 的命令下传输数据。服务器端 DTP 也可以用于“被动”模式，而不是主动在数据端口建立连接。
 * 用户PI（user-PI）：用户协议解析器用 U 端口建立到服务器 FTP 过程的控制连接，并在文件传输时管理用户 DTP。
 * 用户DTP（user-DTP）：数据传输过程在数据端口“监听”服务器 FTP 过程的连接。
 * 控制连接：用户PI 与服务器PI 用来交换命令和响应的信息传输通道。
 * 数据连接：通过控制连接协商的模式和类型进行数据传输。
 */
#pragma once
#include "uvnetpuclic.h"
#include "uvnettcp.h"
#include <string>
#include <stdint.h>

namespace uvNetPlus {
namespace Ftp {
class CUNFtpRequest : public CFtpRequest 
{
public:
    CUNFtpRequest();

    ~CUNFtpRequest();

    /** 删除实例 */
    virtual void Delete();

    /**
     * 改变服务器上的工作目录CWD
     */
    virtual void ChangeWorkingDirectory(std::string path, ResCB cb);

    /**
     * 获取服务器文件列表NLST
     */
    virtual void FileList(ResCB cb);

    /**
     * 获取文件信息或文件列表LIST
     */
    virtual void List(ResCB cb);

    /**
     * 下载文件
     */
    virtual void Download(string file, ResCB cb);

    /**
     * 上传文件
     */
    virtual void Upload(string file, char *data, int size, ResCB cb);

    /* 收到的数据处理 */
    void DoReceive(const char *data, int len);

    /** 发生错误处理 */
    void DoError(string err);

    /** 客户端数据全部发送 */
    void DoDrain();

private:
    CFtpMsg              msg;
    uv_mutex_t           mutex;         //一个实例的多个请求不能并发
    std::string          recvBuff;      //接收数据缓存
};

class CUNFtpClient: public CFtpClient
{

};

}
}