#ifndef __TCP_TEXT_SESSION_H
#define __TCP_TEXT_SESSION_H

#include <memory>
#include "Session.h"

namespace chw {

/**
 * tcp和udp文本模式session，收到数据时打印到控制台
 */
class TextSession : public Session {
public:
    using Ptr = std::shared_ptr<TextSession>();
    TextSession(const Socket::Ptr &sock);
    virtual ~TextSession();

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
     * @param buff 数据
     * @param len  数据长度
     * @return uint32_t 发送成功的数据长度
     */
    uint32_t senddata(uint8_t* buff, uint32_t len) override;

private:
    std::string _cls;
};

}//namespace chw

#endif//__TCP_TEXT_SESSION_H