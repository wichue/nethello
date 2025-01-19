// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __TCP_SAFE_CLIENT_H
#define __TCP_SAFE_CLIENT_H

#include <memory>
#include "Socket.h"
#include "TcpClient.h"

namespace chw {
  
// 封装业务端线程安全的tcp客户端，不使用底层socket锁
// 创建socket和接收数据不需要锁，发生错误关闭连接和数据接收需要锁
class TcpSafeClient : public TcpClient{
public:
    using Ptr = std::shared_ptr<TcpSafeClient>;
    TcpSafeClient(const EventLoop::Ptr &poller = nullptr);
    ~TcpSafeClient();

    // 接收数据回调（epoll线程执行）
    virtual void onRecv(const Buffer::Ptr &pBuf) override;

    // 错误回调
    virtual void onError(const SockException &ex) override;

    int32_t SendData(uint8_t *pData, uint32_t uiLen, uint32_t ip, uint16_t port, bool checkDst);

    // 在业务端发送和接收发生错误时执行
    void DealSocketError(int32_t fd);
private:
    int32_t      _fd;             //保存在业务端的fd
    std::mutex   _mtx;            //线程锁，不要字节对齐
    bool         _bVaildSnd;      //是否已准备好发送数据
};

}//namespace chw


#endif//__TCP_SAFE_CLIENT_H