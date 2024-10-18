#ifndef __TCP_TEXT_CLIENT_H
#define __TCP_TEXT_CLIENT_H

#include <memory>
#include "TcpClient.h"
#include "UdpClient.h"

namespace chw {
template<typename TypeClient>
class TextClient : public TypeClient {
public:
    using Ptr = std::shared_ptr<TextClient>;

    TextClient(const EventLoop::Ptr &poller = nullptr) : TypeClient(poller){};
    virtual ~TextClient() = default;

    // 接收数据回调
    virtual void onRecv(const Buffer::Ptr &pBuf) override
    {
        //接收数据事件
        PrintD("<%s",pBuf->data());
        pBuf->Reset();
    }

    // 错误回调
    virtual void onError(const SockException &ex) override
    {
        //断开连接事件，一般是EOF
        WarnL << ex.what();
    }
};
typedef TextClient<TcpClient> TcpTextClient;
typedef TextClient<UdpClient> UdpTextClient;

}//namespace chw

#endif//__TCP_TEXT_CLIENT_H