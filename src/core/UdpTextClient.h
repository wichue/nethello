#ifndef __UDP_TEXT_CLIENT_H
#define __UDP_TEXT_CLIENT_H

#include <memory>
#include "UdpClient.h"

namespace chw {

class UdpTextClient : public UdpClient {
public:
    using Ptr = std::shared_ptr<UdpTextClient>;

    UdpTextClient(const EventLoop::Ptr &poller = nullptr);
    virtual ~UdpTextClient();

    virtual void onRecv(const Buffer::Ptr &pBuf) override;
    virtual void onError(const SockException &ex) override;
    const Socket::Ptr &getSock() const;
};

}//namespace chw

#endif//__UDP_TEXT_CLIENT_H