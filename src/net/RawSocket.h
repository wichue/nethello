// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __RAW_CLIENT_H
#define __RAW_CLIENT_H

#include <memory>
#include <netpacket/packet.h>
#include "Socket.h"
#include "Client.h"

namespace chw {

/**
 * 原始套接字基类，创建原始套接字。
 * 继承该类，实现虚函数onError、onRecv即可自定义一个raw。
 * 
 */
class RawSocket : public Client {
public:
    using Ptr = std::shared_ptr<RawSocket>;
    RawSocket(const EventLoop::Ptr &poller = nullptr);
    virtual ~RawSocket();

    /**
     * @brief 创建raw socket
     * 
     * @param NetCard   [in]本地网卡名称
     * @return uint32_t 成功返回chw::success，失败返回chw::fail
     */
    virtual uint32_t create_client(const std::string &NetCard, uint16_t, uint16_t localport = 0,const std::string &localip = "0.0.0.0") override;
    
    /**
     * @brief 设置派生类连接结果回调
     * 
     * @param oncon [in]连接结果回调
     */
    virtual void setOnCon(onConCB oncon) override;

protected:
    uint8_t _local_mac[IFHWADDRLEN] = {0};// 本端绑定网卡的MAC地址
    struct sockaddr_ll _local_addr;// 本地网卡地址，也是发送时sendto的目的地址，数据会发送到指定本地网卡
};

} //namespace chw

#endif//__RAW_CLIENT_H