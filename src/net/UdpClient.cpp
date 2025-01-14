﻿// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "UdpClient.h"
#include "util.h"

using namespace std;

namespace chw {

UdpClient::UdpClient(const EventLoop::Ptr &poller) : Client(poller)
{
    setOnCon(nullptr);
}

UdpClient::~UdpClient() {
    TraceL << "~" << getIdentifier();
}

/**
 * @brief 创建udp客户端
 * 
 * @param url       [in]远端ip
 * @param port      [in]远端端口
 * @param localport [in]本地端口，默认0由内核分配
 * @param localip   [in]本地ip，默认"0.0.0.0"(INADDR_ANY)是绑定所有本地ipv4地址
 * @return uint32_t 成功返回chw::success，失败返回chw::fail
 */
uint32_t UdpClient::create_client(const std::string &url, uint16_t port, uint16_t localport,const std::string &localip)
{
    weak_ptr<UdpClient> weak_self = static_pointer_cast<UdpClient>(shared_from_this());
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

    //udp绑定本地IP和端口
    if (!sock_ptr->bindUdpSock(localport, localip.c_str()))
    {
        // udp 绑定端口失败, 可能是由于端口占用或权限问题
        PrintE("Bind udp socket failed, local ip=%s,port=%u,err=%s",localip.c_str(),localport,get_uv_errmsg(true));
        _on_con(SockException(Err_other, std::string("Bind udp socket failed:") + get_uv_errmsg()));

        return chw::fail;
    }

    //udp绑定远端IP和端口
    struct sockaddr_in local_addr;
    local_addr.sin_addr.s_addr = inet_addr(url.c_str());
    local_addr.sin_port = htons(port);
    local_addr.sin_family = AF_INET;
    //chw:客户端使用软绑定
    if(!sock_ptr->bindPeerAddr((struct sockaddr *) &local_addr, sizeof(struct sockaddr_in),true))
    {
        PrintE("bind peer addr failed, remote ip=%s, port=%u",url.c_str(),port);
        _on_con(SockException(Err_other, std::string("bind peer addr failed:") + get_uv_errmsg()));

        return chw::fail;
    }

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

    PrintD("create udp client ,local ip=%s,local port=%d,peer ip=%s,peer port=%d"
        ,getSock()->get_local_ip().c_str(),getSock()->get_local_port(),getSock()->get_peer_ip().c_str(),getSock()->get_peer_port());

    _on_con(SockException(Err_success, "success"));

    return chw::success;
}

/**
 * @brief 设置派生类连接结果回调
 * 
 * @param oncon [in]连接结果回调
 */
void UdpClient::setOnCon(onConCB oncon)
{
    if(oncon)
    {
        _on_con = oncon;
    }
    else
    {
        _on_con = [](const SockException &){
            // 上面已经打印日志了，这里什么都不做
        };
    }
}

} //namespace chw
