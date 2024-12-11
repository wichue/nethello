// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "KcpClient.h"
#include "TimeThread.h"

namespace chw {

KcpClient::KcpClient(const EventLoop::Ptr &poller) : _poller(poller)
{
    _kcp = nullptr;
    _rcv_buf = std::make_shared<Buffer>();
    _rcv_buf->SetCapacity(TCP_BUFFER_SIZE);
    SetRecvUserCb(nullptr);
}

/**
 * @brief 创建kcp实例
 * 
 * @param conv 会话标识
 * @param cb   kcp发送回调，上层决定如何发送数据
 * @param user 上层调用者的指针，kcp发送数据时会传递给KcpSendCb的第四个参数
 */
void KcpClient::CreateKcp(uint32_t conv, KcpSendCb cb, void *user)
{
    ///1.创建kcp实例，两端第一个参数conv要相同
    _kcp = ikcp_create(conv, user);

    ///2.设置kcp发送回调
    _kcp->output = cb;

    ///3.kcp参数设置
    // 配置窗口大小：平均延迟200ms，每20ms发送一个包，
    // 而考虑到丢包重发，设置最大收发窗口为128
    ikcp_wndsize(_kcp, 128, 128);

    // 判断测试用例的模式
    int mode = 2;
    if (mode == 0) {
        // 默认模式
        ikcp_nodelay(_kcp, 0, 10, 0, 0);
    } else if (mode == 1) {
        // 普通模式，关闭流控等
        ikcp_nodelay(_kcp, 0, 10, 0, 1);
    }	else {
        // 启动快速模式
        // 第二个参数 nodelay-启用以后若干常规加速将启动
        // 第三个参数 interval为内部处理时钟，默认设置为 10ms
        // 第四个参数 resend为快速重传指标，设置为2
        // 第五个参数 为是否禁用常规流控，这里禁止
        ikcp_nodelay(_kcp, 2, 10, 2, 1);
        _kcp->rx_minrto = 10;
        _kcp->fastresend = 1;
    }

    ///4.创建定时器周期处理ikcp_update
    std::weak_ptr<KcpClient> weak_self = std::static_pointer_cast<KcpClient>(shared_from_this());
    _timer = std::make_shared<Timer>(0.001, [weak_self]() -> bool {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return false;
        }

        ///5.处理kcp发送和重发
        strong_self->onKcpUpdate();
        return true;
    }, _poller);
}

/**
 * @brief kcp处理接收数据，上层接收到数据时调用
 * 
 * @param buf 数据
 * @param len 数据长度
 */
void KcpClient::RcvDataToKcp(const char* buf, long len)
{
    ///6.预接收数据:调用ikcp_input将裸数据交给KCP，这些数据有可能是KCP控制报文
    int ret = ikcp_input(_kcp, buf, len);
    if(ret < 0)//检测ikcp_input是否提取到真正的数据
    {
        return;
    }

    ///7.kcp将接收到的kcp数据包还原成用户数据
    while (1) {
        int nrecv = ikcp_recv(_kcp, (char*)_rcv_buf->data() + _rcv_buf->Size(), _rcv_buf->Idle());
        // printf("ikcp_recv nrecv=%d\n", nrecv);
        if (nrecv < 0) break;
        _rcv_buf->SetSize(nrecv + _rcv_buf->Size());
        // _FileTransfer->onRecv(_rcv_buf);
        _recv_user_cb(_rcv_buf);
    }

    ///8.每次接收到数据后更新kcp状态(ikcp_update)
    onKcpUpdate();
}

/**
 * @brief 把数据交给kcp发送，上层需要发送数据时调用
 * 
 * @param buffer [in]数据
 * @param len    [in]数据长度
 * @return int   成功返回压入队列的数据长度，0则输入为0或缓存满，失败返回小于0
 */
int KcpClient::KcpSendData(const char *buffer, int len)
{
    //这里只是把数据加入到发送队列
    int ret = ikcp_send(_kcp,buffer,len);
    if(ret >= 0)
    {
        // 通过定时器回调触发 onKcpUpdate 速率更快
        // onKcpUpdate();
    }
    
    return ret;
}

void KcpClient::SetRecvUserCb(const KcpRecvCb& cb)
{
    if(cb != nullptr) {
        _recv_user_cb = std::move(cb);
    } else {
        _recv_user_cb = [](const Buffer::Ptr &buf)->void{
            PrintE("KcpClient not set _recv_user_cb, data ignored: %d",buf->Size());
        };
    }
}

/**
 * @brief 周期执行ikcp_update
 * 
 */
void KcpClient::onKcpUpdate()
{
    // 传入微秒比毫秒速率快很多
    ikcp_update(_kcp,getCurrentMicrosecond());
}

}//namespace chw