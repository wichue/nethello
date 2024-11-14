// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __RAW_PRESS_CLIENT_H
#define __RAW_PRESS_CLIENT_H

#include "RawSocket.h"

namespace chw {

/**
 * 用于原始套接字性能测试，使用自定义以太类型。
 */
class RawPressClient : public  RawSocket {
public:
    using Ptr = std::shared_ptr<RawPressClient>;
    RawPressClient(const EventLoop::Ptr &poller = nullptr);
    ~RawPressClient();

    // 接收数据回调（epoll线程执行）
    virtual void onRecv(const Buffer::Ptr &pBuf) override;

    // 错误回调
    virtual void onError(const SockException &ex) override;

    /**
     * @brief 返回接收包的数量
     * 
     * @return uint64_t 接收包的数量
     */
    uint64_t GetPktNum();

    /**
     * @brief 返回接收包的最大序列号
     * 
     * @return uint64_t 接收包的最大序列号
     */
    uint64_t GetSeq();

    /**
     * @brief 返回接收字节总大小
     * 
     * @return uint64_t 接收字节总大小
     */
    uint64_t GetRcvLen();
private:
    uint64_t _rcv_num;// 接收包的数量
    uint64_t _rcv_seq;// 接收包的最大序列号,raw是否会乱序?
    uint64_t _rcv_len;// 接收的字节总大小
};

}//namespace chw

#endif//__RAW_PRESS_CLIENT_H