#ifndef __TCP_TEXT_SESSION_H
#define __TCP_TEXT_SESSION_H

#include <memory>
#include "Session.h"

namespace chw {

class TcpTextSession : public Session {
public:
    using Ptr = std::shared_ptr<TcpTextSession>();
    TcpTextSession(const Socket::Ptr &sock);
    virtual ~TcpTextSession();

    void onRecv(const Buffer::Ptr &buf) override;
    void onError(const SockException &err) override;
    void onManager() override;
    const std::string &className() const override;

    /**
     * @brief tcp发送数据
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