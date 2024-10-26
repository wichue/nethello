#ifndef __TCP_TEXT_SESSION_H
#define __TCP_TEXT_SESSION_H

#include <memory>
#include "Session.h"

namespace chw {

/**
 * 文件传输会话，用于服务端接收文件。
 */
class FileSession : public Session {
public:
    using Ptr = std::shared_ptr<FileSession>();
    FileSession(const Socket::Ptr &sock);
    virtual ~FileSession();

    /**
     * @brief 接收数据回调（epoll线程执行）
     * 
     * @param buf [in]数据
     */
    void onRecv(const Buffer::Ptr &buf) override;

    /**
     * @brief 发生错误时的回调
     * 
     * @param err [in]异常
     */
    void onError(const SockException &err) override;

    /**
     * @brief 定时器周期管理回调
     * 
     */
    void onManager() override;

    /**
     * @brief 返回当前类名称
     * 
     * @return const std::string& 类名称
     */
    const std::string &className() const override;

    virtual uint64_t GetPktNum()override{return 0;};
    virtual uint64_t GetSeq()override{return 0;};
    virtual uint64_t GetRcvLen()override{return 0;};

    /**
     * @brief tcp发送数据（可在任意线程执行）
     * 
     * @param buff [in]数据
     * @param len  [in]数据长度
     * @return uint32_t 发送成功的数据长度
     */
    uint32_t senddata(uint8_t* buff, uint32_t len) override;

private:
    /**
     * @brief 接收传输结束消息
     * 
     * @param buf [in]数据
     */
    void procTranEnd(char* buf);

    /**
     * @brief 接收文件传输请求消息
     * 
     * @param buf [in]数据
     */
    void procTranReq(char* buf);

    /**
     * @brief 接收文件传输数据
     * 
     * @param buf [in]数据
     * @param len [in]长度
     */
    void procFileData(char* buf, uint32_t len);

    /**
     * @brief 发送信令消息给对端
     * 
     * @param msgtype [in]消息类型
     * @param code    [in]错误码
     */
    void SendSignalMsg(uint32_t msgtype, uint32_t code);

    /**
     * @brief 分发消息
     * 
     * @param buf [in]消息
     * @param len [in]长度
     */
    void DispatchMsg(char* buf, uint32_t len);
private:
    std::string _cls;// 类名
    uint32_t _status;//FileTranStatus

    std::string _filepath;// 文件保存路经
    std::string _filename;// 文件保存文件名
    uint32_t _filesize; // 文件大小

    FILE* _write_file;// 写文件句柄
    uint32_t _write_size;// 已写入的大小
};

}//namespace chw

#endif//__TCP_TEXT_SESSION_H