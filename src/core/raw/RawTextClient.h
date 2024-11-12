// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __RAW_TEXT_CLIENT_H
#define __RAW_TEXT_CLIENT_H

#include "RawSocket.h"

namespace chw {

/**
 * 用于原始套接字之间收发文本消息，使用自定义以太类型。
 */
class RawTextClient : public  RawSocket {
public:
    using Ptr = std::shared_ptr<RawTextClient>;
    RawTextClient(const EventLoop::Ptr &poller = nullptr);
    ~RawTextClient();

    // 接收数据回调（epoll线程执行）
    virtual void onRecv(const Buffer::Ptr &pBuf) override;

    // 错误回调
    virtual void onError(const SockException &ex) override;

    /**
     * @brief 使用原始套接字发送文本数据
     * 
     * @param buf 数据
     * @param len 数据长度
     * @return uint32_t 发送成功的数据长度，0则是发生了错误
     */
    uint32_t send_text(char* buf, uint32_t len);
};

}//namespace chw

#endif//__RAW_TEXT_CLIENT_H