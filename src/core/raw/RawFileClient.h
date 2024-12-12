// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __RAW_FILE_CLIENT_H
#define __RAW_FILE_CLIENT_H

#include <memory>
#include "RawSocket.h"
#include "ikcp.h"
#include "FileSend.h"
#include "FileRecv.h"
#include "FileTransfer.h"
#include "KcpClient.h"

namespace chw {

/**
 * @brief 基于linux原始套接字的文件传输发送端，使用kcp实现可靠传输
 * 
 */
class RawFileClient : public  RawSocket {
public:
    using Ptr = std::shared_ptr<RawFileClient>;
    RawFileClient(const chw::EventLoop::Ptr& poller = nullptr);
    ~RawFileClient();

    // 接收数据回调（epoll线程执行）
    virtual void onRecv(const Buffer::Ptr &pBuf) override;

    // 错误回调
    virtual void onError(const SockException &ex) override;

    /**
     * @brief 创建kcp实例
     * 
     * @param conv [in]会话标识
     */
    void CreateAKcp(uint32_t conv);

    /**
     * @brief 开始文件传输，客户端调用
     * 
     */
    void StartFileTransf();

    /**
     * @brief 周期打印收发速率
     * 
     */
    void onManager();
private:
    /**
     * @brief 从kcp接收用户数据
     * 
     * @param buf [in]数据
     */
    void RcvDataFromKcp(const Buffer::Ptr &buf);

    /**
     * @brief 把数据交给kcp发送，需要发送数据时调用
     * 
     * @param buffer [in]数据
     * @param len    [in]数据长度
     * @return int   成功返回压入队列的数据长度，0则输入为0或缓存满，失败返回小于0
     */
    int SendDataToKcpQue(const char *buffer, int len);
private:
    FileTransfer::Ptr _FileTransfer;// 文件传输业务
    std::shared_ptr<Timer> _timer_spd;// 周期输出信息定时器
    KcpClient::Ptr _KcpClient;// kcp客户端
};

}//namespace chw

#endif//__RAW_FILE_CLIENT_H