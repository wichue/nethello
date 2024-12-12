// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __FILE_TCP_CLIENT_H
#define __FILE_TCP_CLIENT_H

#include <memory>
#include "TcpClient.h"
#include "ComProtocol.h"
#include "EventLoop.h"
#include "FileSend.h"

namespace chw {

/**
 * 文件传输tcp Client，发送文件。
 */
class FileTcpClient : public TcpClient {
public:
    using Ptr = std::shared_ptr<FileTcpClient>;

    FileTcpClient(const EventLoop::Ptr &poller = nullptr);
    virtual ~FileTcpClient() = default;

    // 接收数据回调（epoll线程执行）
    virtual void onRecv(const Buffer::Ptr &pBuf) override;

    // 错误回调
    virtual void onError(const SockException &ex) override;

    int SendDataToSer(const char *buffer, int len);

    /**
     * @brief 开始文件传输
     * 
     */
    void StartTransf();
private:
    FileTransfer::Ptr _FileTransfer;// 文件传输业务
};

}//namespace chw

#endif//__FILE_TCP_CLIENT_H