// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __RAW_FILE_CLIENT_H
#define __RAW_FILE_CLIENT_H

#include <memory>
#include "RawSocket.h"
#include "ikcp.h"
#include "FileSend.h"

namespace chw {

typedef std::function<int(const char *buffer, int len, ikcpcb *kcp, void *user)> RawSndCb;

class RawFileClient : public  RawSocket {
public:
    RawFileClient(const chw::EventLoop::Ptr& poller = nullptr);
    ~RawFileClient();

    // 接收数据回调（epoll线程执行）
    virtual void onRecv(const Buffer::Ptr &pBuf) override;

    // 错误回调
    virtual void onError(const SockException &ex) override;

    void CreateKcp();
private:
    /**
     * @brief 原始套结字发送数据回调，传递给kcp发送数据，不直接调用
     * 
     * @param buf 
     * @param len 
     * @param kcp 
     * @param user 
     * @return int 
     */
    int RawSendDataCb(const char *buf, int len, ikcpcb *kcp, void *user);

    /**
     * @brief 把数据交给kcp发送，需要发送数据时调用
     * 
     * @param buffer 数据
     * @param len    数据长度
     * @return int   成功返回压入队列的数据长度，0则输入为0或缓存满，失败返回小于0
     */
    int KcpSendData(const char *buffer, int len);

private:
    ikcpcb *_kcp;
    FileSend::Ptr _FileSend;

    RawSndCb _RawSndCb;// 用于把std::function转换为函数指针
};

}//namespace chw

#endif//__RAW_FILE_CLIENT_H