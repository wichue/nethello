// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "RawSocket.h"
#include "util.h"
#include <linux/if_ether.h>

using namespace std;

namespace chw {

RawSocket::RawSocket(const EventLoop::Ptr &poller) : Client(poller)
{
    setOnCon(nullptr);
}

RawSocket::~RawSocket() {

}

/**
 * @brief 创建raw socket
 * 
 * @param NetCard   [in]本地网卡名称
 * @return uint32_t 成功返回chw::success，失败返回chw::fail
 */
uint32_t RawSocket::create_client(const std::string &NetCard, uint16_t , uint16_t ,const std::string &)
{
    weak_ptr<RawSocket> weak_self = static_pointer_cast<RawSocket>(shared_from_this());
    _socket = Socket::createSocket(_poller);

    auto sock_ptr = _socket.get();

    // 设置错误回调
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

    // 创建原始套接字
    int32_t fd = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
    if(fd < 0)
    {
        PrintE("create raw socket failed:%s",get_uv_errmsg());
        return chw::fail;
    }

    // 设置接收回调
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

    //todo:设置收发缓存大小

    if(NetCard.empty())
    {
        PrintD("create raw socket, local interface:%s, mac:%s.",NetCard.c_str(),MacBuftoStr(_local_mac).c_str());
        sock_ptr->fromSock(fd, SockNum::Sock_UDP);
        _on_con(SockException(Err_success, "success"));
        return chw::success;
    }

    // 获取网卡flags
    struct ifreq ifr;
    strcpy(ifr.ifr_name, NetCard.c_str());
    if (ioctl(fd, SIOCGIFFLAGS, &ifr)){
        PrintE("ioctl SIOCGIFFLAGS failed:%s,name=%s",get_uv_errmsg(),NetCard.c_str());
        close(fd);
        return chw::fail;
    }

    // 设置网卡混杂模式
    ifr.ifr_flags |= IFF_PROMISC;
    if (ioctl(fd, SIOCSIFFLAGS, &ifr)) {
        PrintE("ioctl SIOCSIFFLAGS failed:%s",get_uv_errmsg());
        close(fd);
        return chw::fail;
    }

    // 获取网卡序号
    if(ioctl(fd,SIOCGIFINDEX,&ifr)) {
        PrintE("ioctl SIOCGIFINDEX failed:%s",get_uv_errmsg());
        close(fd);
        return chw::fail;
    }

    // 设置本地网卡地址
    memset(&_local_addr,0,sizeof(_local_addr));
    _local_addr.sll_family = PF_PACKET;
    _local_addr.sll_ifindex = ifr.ifr_ifindex;
    _local_addr.sll_protocol = htons(ETH_P_ALL);

    // 获取网卡MAC地址
    if(ioctl(fd,SIOCGIFHWADDR,&ifr)) {
        PrintE("ioctl SIOCGIFHWADDR failed:%s",get_uv_errmsg());
        close(fd);
        return chw::fail;
    }
    memcpy(_local_mac,ifr.ifr_hwaddr.sa_data,IFHWADDRLEN);

    // 不执行bind也可以收发
    if(bind(fd,(struct sockaddr*)&_local_addr,sizeof(_local_addr))) {
        PrintE("bind raw socket,fd:%d, failed:%s",fd,get_uv_errmsg());
        close(fd);
        return chw::fail;
    }

    // 设置发送和接收缓存区大小,某些场景下对速率有明显提升
    // SockUtil::setSendBuf(fd,100*1024*1024);
    // SockUtil::setRecvBuf(fd,100*1024*1024);

    PrintD("create raw socket, local interface:%s, mac:%s.",NetCard.c_str(),MacBuftoStr(_local_mac).c_str());

    sock_ptr->fromSock(fd, SockNum::Sock_UDP);
    _on_con(SockException(Err_success, "success"));

    return chw::success;
}

/**
 * @brief 设置派生类连接结果回调
 * 
 * @param oncon [in]连接结果回调
 */
void RawSocket::setOnCon(onConCB oncon)
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
