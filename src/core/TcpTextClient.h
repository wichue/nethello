#ifndef __TCP_TEXT_CLIENT_H
#define __TCP_TEXT_CLIENT_H

#include <memory>
#include "TcpClient.h"


namespace chw {
template<typename TypeClient>
class TextClient : public TypeClient {
public:
    using Ptr = std::shared_ptr<TextClient>;

    TextClient(const EventLoop::Ptr &poller = nullptr) : TcpClient(poller){};
    virtual ~TextClient() = default;

    // 连接结果事件
    virtual void onConnect(const SockException &ex) override
    {
    if(ex)
    {
        PrintE("tcp connect failed, please check ip and port, ex:%s.", ex.what());
        sleep_exit(100*1000);
    }
    else
    {
        PrintD("connect success.");
        usleep(1000);
        InfoLNCR << ">";
    }
    }

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

}//namespace chw

#endif//__TCP_TEXT_CLIENT_H