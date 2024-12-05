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

namespace chw {

typedef std::function<int(const char *buffer, int len, ikcpcb *kcp, void *user)> RawSndCb;

/**
 * @brief 基于linux原始套接字的文件传输发送端，使用kcp实现可靠传输
 * 发送数据使用： KcpSendData
 * 接收数据使用： KcpRcvData
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
     * @param conv 会话标识
     */
    void CreateKcp(uint32_t conv);

    /**
     * @brief 开始文件传输，客户端调用
     * 
     */
    void StartFileTransf();
private:
    /**
     * @brief kcp处理接收数据
     * 
     * @param buf 数据
     * @param len 数据长度
     */
    void KcpRcvData(const char* buf, long len);

    /**
     * @brief 原始套结字发送数据回调，传递给kcp发送数据，不直接调用
     * 
     * @param buf   [in]数据
     * @param len   [in]数据长度
     * @param kcp   [in]kcp实例
     * @param user  [in]用户句柄
     * @return int  发送成功的数据长度
     */
    // int RawSendDataCb(const char *buf, int len, ikcpcb *kcp, void *user);

    /**
     * @brief 把数据交给kcp发送，需要发送数据时调用
     * 
     * @param buffer [in]数据
     * @param len    [in]数据长度
     * @return int   成功返回压入队列的数据长度，0则输入为0或缓存满，失败返回小于0
     */
    int KcpSendData(const char *buffer, int len);

    /**
     * @brief 周期执行ikcp_update
     * 
     */
    void onKcpUpdate();

private:
    ikcpcb *_kcp;// kcp实例
    FileTransfer::Ptr _FileTransfer;// 文件发送业务
    std::shared_ptr<Timer> _timer;// 周期执行ikcp_update定时器

    RawSndCb _RawSndCb;// 用于把std::function转换为函数指针
};

}//namespace chw

#endif//__RAW_FILE_CLIENT_H