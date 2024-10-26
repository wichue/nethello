// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __PRESS_CLIENT_H
#define __PRESS_CLIENT_H

#include <memory>
#include "TcpClient.h"
#include "UdpClient.h"

namespace chw {

/**
 * 压力测试Client，正常情况下只发送不接收。
 * tcp和udp客户端业务功能类似，使用类模板实现。
 */
template<typename TypeClient>
class PressClient : public TypeClient {
public:
    using Ptr = std::shared_ptr<PressClient>;

    PressClient(const EventLoop::Ptr &poller = nullptr) : TypeClient(poller){};
    virtual ~PressClient() = default;

    // 接收数据回调（epoll线程执行）
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
typedef PressClient<TcpClient> PressTcpClient;
typedef PressClient<UdpClient> PressUdpClient;

}//namespace chw

#endif//__PRESS_CLIENT_H