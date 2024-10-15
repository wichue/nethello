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
    const std::string &className() const;

private:
    std::string _cls;
};

}//namespace chw

#endif//__TCP_TEXT_SESSION_H