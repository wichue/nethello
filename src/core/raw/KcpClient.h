// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __KCP_CLIENT_H
#define __KCP_CLIENT_H

#include <memory>
#include "ikcp.h"
#include "EventLoop.h"
#include "Timer.h"
#include "Buffer.h"

namespace chw {

typedef int (*KcpSendCb)(const char *, int , ikcpcb *, void *);// kcp发送回调，上层设置如何发送数据
typedef std::function<void(const Buffer::Ptr &)> KcpRecvCb;// kcp接收回调，把还原的用户数据交给上层

/**
 * @brief 定义kcp客户端，对原生kcp接口进行封装
 * 
 */
class KcpClient : public std::enable_shared_from_this<KcpClient> {
public:
    using Ptr = std::shared_ptr<KcpClient>;
    KcpClient(const EventLoop::Ptr &poller);
    virtual ~KcpClient() = default;

    /**
     * @brief 创建kcp实例
     * 
     * @param conv [in]会话标识
     * @param cb   [in]kcp发送回调，上层决定如何发送数据
     * @param user [in]上层调用者的指针，kcp发送数据时会传递给KcpSendCb的第四个参数
     */
    void CreateKcp(uint32_t conv, KcpSendCb cb, void *user);

    /**
     * @brief kcp处理接收数据，上层接收到数据时调用
     * 
     * @param buf [in]数据
     * @param len [in]数据长度
     */
    void RcvDataToKcp(const char* buf, long len);

    /**
     * @brief 把数据交给kcp发送，上层需要发送数据时调用
     * 
     * @param buffer [in]数据
     * @param len    [in]数据长度
     * @return int   成功返回压入队列的数据长度，0则输入为0或缓存满，失败返回小于0
     */
    int KcpSendData(const char *buffer, int len);

    void SetRecvUserCb(const KcpRecvCb& cb);
private:
    /**
     * @brief 周期执行ikcp_update
     * 
     */
    void onKcpUpdate();
private:
    ikcpcb *_kcp;// kcp实例
    Buffer::Ptr _rcv_buf;// 存放接收到数据后，kcp还原的用户数据
    EventLoop::Ptr _poller;// 绑定的事件循环
    std::shared_ptr<Timer> _timer;// 周期执行ikcp_update定时器

    KcpRecvCb _recv_user_cb;// 用户原始数据接收回调
    std::mutex _mtx;// 发送和接收在不同线程时，需要加锁
};

}//namespace chw


#endif//__KCP_CLIENT_H