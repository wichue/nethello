// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "Client.h"
#include <atomic>

namespace chw {

Client::Client(const EventLoop::Ptr &poller) : _poller(poller)
{

}

/**
 * 主动断开连接
 * @param ex [in]触发onErr事件时的参数
 */
void Client::shutdown(const SockException &ex)
{
    _socket->shutdown(ex);
}

/**
 * 连接中或已连接返回true，断开连接时返回false
 */
bool Client::alive() const
{
    return _socket && _socket->alive();
}

/**
 * 返回唯一标识
 */
std::string Client::getIdentifier() const
{
    if (_id.empty()) {
        static std::atomic<uint64_t> s_index { 0 };
        _id = chw::demangle(typeid(*this).name()) + "-" + std::to_string(++s_index);
    }
    return _id;
}

const Socket::Ptr &Client::getSock() const
{
    return _socket;
}

/**
 * @brief 发送数据（任意线程执行）
 * 
 * @param buff [in]数据
 * @param len  [in]数据长度
 * @return uint32_t 发送成功的数据长度
 */
uint32_t Client::senddata(char* buff, uint32_t len)
{
    if(_socket) {
        return _socket->send_i(buff,len);
    } else {
        return 0;
    }
}

}//namespace chw 