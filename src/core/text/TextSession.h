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
     * @param buf 
     */
    void onRecv(const Buffer::Ptr &buf) override;
    void onError(const SockException &err) override;
    void onManager() override;
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