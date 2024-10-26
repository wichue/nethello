// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __TCP_CLIENT_H
#define __TCP_CLIENT_H

#include <memory>
#include "Socket.h"
#include "Client.h"

namespace chw {

/**
 * Tcp客户端基类，实现连接等接口。
 * 继承该类，实现虚函数onConnect、onError、onRecv即可自定义一个tcp客户端。
 */
class TcpClient : public Client {
public:
    using Ptr = std::shared_ptr<TcpClient>;
    TcpClient(const EventLoop::Ptr &poller = nullptr);
    virtual ~TcpClient();

    /**
     * 开始连接tcp服务器（任意线程执行）
     * @param url           [in]服务器ip或域名
     * @param port          [in]服务器端口
     * @param timeout_sec   [in]超时时间,单位秒
     * @param local_port    [in]本地端口
     * @return uint32_t 成功返回chw::success，失败返回chw::fail
     */
    virtual uint32_t create_client(const std::string &url, uint16_t port, uint16_t localport = 0,const std::string &localip = "::") override;

    /**
     * @brief 设置派生类连接结果回调
     * 
     * @param oncon [in]连接结果回调
     */
    virtual void setOnCon(onConCB oncon) override;
protected:
    /**
     * @brief 派生类连接结果事件（epoll线程执行）
     * 
     * @param ex [in]结果
     */
    virtual void onConnect(const SockException &ex) override;
private:
    /**
     * @brief 连接结果回调，成功则设置接收回调
     * 
     * @param ex [in]结果
     */
    void onSockConnect(const SockException &ex);
private:
    onConCB _on_con;// 派生类连接结果回调
};

} //namespace chw
#endif //__TCP_CLIENT_H
