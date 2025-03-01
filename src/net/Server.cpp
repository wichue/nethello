// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "Server.h"

namespace chw {

Server::Server(const EventLoop::Ptr &poller) : _poller(poller)
{

}

/**
 * @brief 返回服务端的Socket
 * 
 * @return const Socket::Ptr& 
 */
const Socket::Ptr &Server::getSock() const
{
    return _socket;
}

/**
 * @brief 允许自定义socket构建行为,默认使用 Socket::createSocket
 */
void Server::setOnCreateSocket(Socket::onCreateSocket cb)
{
    if (cb) {
    _on_create_socket = std::move(cb);
    } else {
    _on_create_socket = [](const EventLoop::Ptr &poller) {
        return Socket::createSocket(poller, false);
        };
    }
}

/**
 * @brief 获取服务器监听端口号, 服务器可以选择监听随机端口
 */
uint16_t Server::getPort() 
{
    if (!_socket) {
        return 0;
    }
    return _socket->get_local_port();
}

/**
 * @brief 创建服务端socket
 */
Socket::Ptr Server::createSocket(const EventLoop::Ptr &poller)
{
    return _on_create_socket(poller);
}

}//namespace chw