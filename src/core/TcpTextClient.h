#ifndef __TCP_TEXT_CLIENT_H
#define __TCP_TEXT_CLIENT_H

#include <memory>
#include "TcpClient.h"

namespace chw {

class TcpTextClient : public TcpClient {
public:
    using Ptr = std::shared_ptr<TcpTextClient>;

    TcpTextClient(const EventLoop::Ptr &poller = nullptr);
    virtual ~TcpTextClient();

    virtual void onConnect(const SockException &ex) override;
    virtual void onRecv(const Buffer::Ptr &pBuf) override;
    virtual void onError(const SockException &ex) override;
    const Socket::Ptr &getSock() const;
};

}//namespace chw

#endif//__TCP_TEXT_CLIENT_H