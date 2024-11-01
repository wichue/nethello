// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __FILE_TCP_CLIENT_H
#define __FILE_TCP_CLIENT_H

#include <memory>
#include "TcpClient.h"
#include "ComProtocol.h"
#include "EventLoop.h"

namespace chw {

/**
 * 文件传输tcp Client，发送文件。
 */
class FileTcpClient : public TcpClient {
public:
    using Ptr = std::shared_ptr<FileTcpClient>;

    FileTcpClient(const EventLoop::Ptr &poller = nullptr);
    virtual ~FileTcpClient() = default;

    /**
     * @brief 分发消息
     * 
     * @param buf [in]消息
     * @param len [in]长度
     */
    void DispatchMsg(char* buf, uint32_t len);

    // 接收数据回调（epoll线程执行）
    virtual void onRecv(const Buffer::Ptr &pBuf) override;

    // 错误回调
    virtual void onError(const SockException &ex) override;

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
    EventLoop::Ptr _send_poller;
};

}//namespace chw

#endif//__FILE_TCP_CLIENT_H