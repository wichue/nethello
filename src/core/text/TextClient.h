// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __TCP_TEXT_CLIENT_H
#define __TCP_TEXT_CLIENT_H

#include <memory>
#include "TcpClient.h"
#include "UdpClient.h"

namespace chw {

/**
 * 文本模式Client，收到数据时打印到控制台。
 * tcp和udp客户端业务功能相同，使用类模板实现。
 */
template<typename TypeClient>
class TextClient : public TypeClient {
public:
    using Ptr = std::shared_ptr<TextClient>;

    TextClient(const EventLoop::Ptr &poller = nullptr) : TypeClient(poller){};
    virtual ~TextClient() = default;

    // 接收数据回调（epoll线程执行）
    virtual void onRecv(const Buffer::Ptr &pBuf) override
    {
        //接收数据事件
        PrintD("<%s",pBuf->data());
        pBuf->Reset0();
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