// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __UDP_CLIENT_H
#define __UDP_CLIENT_H

#include <memory>
#include "Socket.h"
#include "Client.h"

namespace chw {

/**
 * Udp客户端基类，实现创建客户端接口。
 * 继承该类，实现虚函数onError、onRecv即可自定义一个udp客户端。
 */
class UdpClient : public Client {
public:
    using Ptr = std::shared_ptr<UdpClient>;
    UdpClient(const EventLoop::Ptr &poller = nullptr);
    virtual ~UdpClient();

    /**
     * @brief 创建udp客户端
     * 
     * @param url       [in]远端ip
     * @param port      [in]远端端口
     * @param localport [in]本地端口，默认0由内核分配
     * @param localip   [in]本地ip，默认"0.0.0.0"(INADDR_ANY)是绑定所有本地ipv4地址
     * @return uint32_t 成功返回chw::success，失败返回chw::fail
     */
    virtual uint32_t create_client(const std::string &url, uint16_t port, uint16_t localport = 0,const std::string &localip = "0.0.0.0") override;
    
    /**
     * @brief 设置派生类连接结果回调
     * 
     * @param oncon [in]连接结果回调
     */
    virtual void setOnCon(onConCB oncon) override;
};

} //namespace chw
#endif //__UDP_CLIENT_H
