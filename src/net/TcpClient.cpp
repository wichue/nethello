// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "TcpClient.h"

using namespace std;

namespace chw {

TcpClient::TcpClient(const EventLoop::Ptr &poller) : Client(poller)
{
    _poller = poller;
    setOnCon(nullptr);
}

TcpClient::~TcpClient() {
    TraceL << "~" << TcpClient::getIdentifier();
}

/**
 * @brief 创建tcp客户端
 * 
 * @param url       [in]远端ip
 * @param port      [in]远端端口
 * @param localport [in]本地端口，默认0由内核分配
 * @param localip   [in]本地ip，默认"0.0.0.0"(INADDR_ANY)是绑定所有本地ipv4地址
 * @return uint32_t 成功返回chw::success，失败返回chw::fail
 */
uint32_t TcpClient::create_client(const string &url, uint16_t port, uint16_t localport, const std::string &localip) {
    float timeout_sec = 5;
    weak_ptr<TcpClient> weak_self = static_pointer_cast<TcpClient>(shared_from_this());
    _socket = Socket::createSocket(_poller);

    auto sock_ptr = _socket.get();
    sock_ptr->setOnErr([weak_self, sock_ptr](const SockException &ex) {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }
        if (sock_ptr != strong_self->_socket.get()) {
            //已经重连socket，上次的socket的事件忽略掉
            return;
        }
        // strong_self->_timer.reset();
        TraceL << strong_self->getIdentifier() << " on err: " << ex;
        strong_self->onError(ex);
    });

    //TraceL << getIdentifier() << " start connect " << url << ":" << port;
    sock_ptr->connect(url, port, [weak_self](const SockException &err) {
        auto strong_self = weak_self.lock();
        if (strong_self) {
            strong_self->onSockConnect(err);
        }
    }, timeout_sec, localip, localport);

    return chw::success;
}

/**
 * @brief 设置派生类连接结果回调
 * 
 * @param oncon [in]连接结果回调
 */
void TcpClient::setOnCon(onConCB oncon)
{
    if(oncon)
    {
        _on_con = oncon;
    }
    else
    {
        _on_con = [](const SockException &ex){
            if(ex)
            {   
                PrintE("tcp connect failed, please check ip and port, ex:%s.", ex.what());
            }
            else
            {   
                PrintD("connect success.");
            }
        };
    }
}

/**
 * @brief 连接结果回调，成功则设置接收回调
 * 
 * @param ex [in]结果
 */
void TcpClient::onSockConnect(const SockException &ex) {
    // TraceL << getIdentifier() << " connect result: " << ex;//chw
    if (ex) {
        //连接失败
        // _timer.reset();
        _on_con(ex);
        return;
    }

    auto sock_ptr = _socket.get();
    weak_ptr<TcpClient> weak_self = static_pointer_cast<TcpClient>(shared_from_this());

    sock_ptr->setOnRead([weak_self, sock_ptr](const Buffer::Ptr &pBuf, struct sockaddr *, int) {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }
        if (sock_ptr != strong_self->_socket.get()) {
            //已经重连socket，上传socket的事件忽略掉
            return;
        }
        try {
            strong_self->onRecv(pBuf);
        } catch (std::exception &ex) {
            strong_self->shutdown(SockException(Err_other, ex.what()));
        }
    });

    _on_con(ex);
}

} //namespace chw
