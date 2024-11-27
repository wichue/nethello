// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __FILE_SEND_H
#define __FILE_SEND_H

#include <memory>
#include "TcpClient.h"
#include "ComProtocol.h"
#include "EventLoop.h"

namespace chw {

/**
 * 文件传输发送端封装，只关注文件传输的信令和数据交互，不关注数据如何发送和接收。
 * 支持tcp、upd+kcp、raw+kcp等多种模式。
 * 使用时调用SetSndData设置发送函数，把接收的数据传递给onRecv。
 */
class FileSend : public std::enable_shared_from_this<FileSend> {
public:
    using Ptr = std::shared_ptr<FileSend>;

    FileSend(const EventLoop::Ptr &poller = nullptr);
    virtual ~FileSend() = default;

    void SetSndData(std::function<uint32_t(char* buf, uint32_t )> cb);

    /**
     * @brief 分发消息
     * 
     * @param buf [in]消息
     * @param len [in]长度
     */
    void DispatchMsg(char* buf, uint32_t len);

    // 接收数据回调（epoll线程执行）
    void onRecv(const Buffer::Ptr &pBuf);

    // 错误回调
    void onError(const SockException &ex);

    /**
     * @brief 开始文件传输
     * 
     */
    void StartTransf();
private:
    /**
     * @brief 收到文件发送响应
     * 
     * @param pBuf [in]响应数据
     */
    void procTranRsp(char* pBuf);

    /**
     * @brief 收到传输结束消息
     * 
     * @param pBuf [in]消息
     */
    void procTranEnd(char* pBuf);

    /**
     * @brief 本端文件发送结束
     * 
     * @param status 状态码(FileTranStatus)
     */
    void localTransEnd(uint32_t status, uint32_t filesiez, double times);

private:
    uint32_t _status;//FileTranStatus
    EventLoop::Ptr _send_poller;// 文件发送线程
    EventLoop::Ptr _poller;// 调用者线程
    std::function<uint32_t(char*, uint32_t )> _send_data;// 发送数据函数
};

}//namespace chw

#endif//__FILE_SEND_H