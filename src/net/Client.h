// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __CLIENT_H
#define __CLIENT_H

#include <memory>
#include <functional>
#include "Socket.h"

namespace chw {

/**
 * udp和tcp客户端抽象基类类，实现一些基础功能，成员：Socket::Ptr，EventLoop::Ptr;
 */
class Client : public std::enable_shared_from_this<Client> {
public:
    using Ptr = std::shared_ptr<Client>;
    using onConCB = std::function<void(const SockException &ex)>;
    Client(const EventLoop::Ptr &poller);
    virtual ~Client() = default;

    /**
     * @brief 创建客户端
     * 
     * @param url       [in]远端ip
     * @param port      [in]远端端口
     * @param localport [in]本地端口，默认0由内核分配
     * @param localip   [in]本地ip，默认"0.0.0.0"(INADDR_ANY)是绑定所有本地ipv4地址
     * @return uint32_t 成功返回chw::success，失败返回chw::fail
     */
    virtual uint32_t create_client(const std::string &url, uint16_t port, uint16_t localport = 0,const std::string &localip = "0.0.0.0") = 0;

    /**
     * @brief 设置连接结果回调
     * 
     * @param oncon [in]连接结果回调
     */
    virtual void setOnCon(onConCB oncon) = 0;

    /**
     * 主动断开连接
     * @param ex [in]触发onErr事件时的参数
     */
    void shutdown(const SockException &ex = SockException(Err_shutdown, "self shutdown"));

    /**
     * 连接中或已连接返回true，断开连接时返回false
     */
    virtual bool alive() const;

    /**
     * 返回唯一标识
     */
    std::string getIdentifier() const;

    /**
     * @brief 发送数据（任意线程执行）
     * 
     * @param buff [in]数据
     * @param len  [in]数据长度
     * @return uint32_t 发送成功的数据长度
     */
    uint32_t senddata_i(char* buff, uint32_t len);

    /**
     * @brief 发送数据（任意线程执行）
     * 
     * @param buff [in]数据
     * @param len  [in]数据长度
     * @return uint32_t 成功返回chw::success,失败返回chw::fail
     */
    uint32_t senddata_b(char* buff, uint32_t len);

    /**
     * @brief 使用网络地址发送数据，用于udp和raw
     * 
     * @param buff 数据
     * @param len  数据长度
     * @param addr 目的地址
     * @param socklen 目的地址长度
     * @return uint32_t 发送成功的数据长度
     */
    uint32_t send_addr(char* buff, uint32_t len, struct sockaddr* addr, int32_t socklen);

    const Socket::Ptr &getSock() const;

protected:
    /**
     * 派生类收到 eof 或其他导致脱离 Server 事件的回调
     * 收到该事件时, 该对象一般将立即被销毁
     * @param err [in]原因
     */
    virtual void onError(const SockException &err) = 0;

    /**
     * 派生类接收数据入口
     * @param buf [in]数据
     */
    virtual void onRecv(const Buffer::Ptr &buf) = 0;

protected:
    mutable std::string _id;// 唯一标识
    Socket::Ptr _socket;// 客户端Socket
    EventLoop::Ptr _poller;// 绑定的事件循环
    onConCB _on_con;// 派生类tcp连接结果回调，或udp创建socket回调
};


}//namespace chw 


#endif//__CLIENT_H