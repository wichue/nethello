// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "RawTextClient.h"
#include "ComProtocol.h"
#include "MemoryHandle.h"
#include "GlobalValue.h"

namespace chw {

RawTextClient::RawTextClient(const EventLoop::Ptr &poller) : RawSocket(poller)
{

}

RawTextClient::~RawTextClient()
{

}

// 接收数据回调（epoll线程执行）
void RawTextClient::onRecv(const Buffer::Ptr &pBuf)
{
    ethhdr* peth = (ethhdr*)pBuf->data();
    uint16_t uEthType = ntohs(peth->h_proto);
    switch(uEthType)
    {
        case ETH_RAW_TEXT:
        {
            //接收数据事件
            PrintD("\b<%s",(char*)(pBuf->data()) + sizeof(ethhdr));
            InfoLNCR << ">";
        }
        break;

        default:
        break;
    }

    pBuf->Reset0();
}

// 错误回调
void RawTextClient::onError(const SockException &ex)
{
    //断开连接事件，一般是EOF
    WarnL << ex.what();
}

/**
 * @brief 使用原始套接字发送文本数据
 * 
 * @param buf 数据
 * @param len 数据长度
 * @return uint32_t 发送成功的数据长度，0则是发生了错误
 */
uint32_t RawTextClient::send_text(char* buf, uint32_t len)
{
    char* snd_buf = (char*)_RAM_NEW_(sizeof(ethhdr) + len);
    memcpy(snd_buf + sizeof(ethhdr),buf,len);

    ethhdr* peth = (ethhdr*)snd_buf;
    memcpy(peth->h_dest,gConfigCmd.dstmac,IFHWADDRLEN);
    memcpy(peth->h_source,_local_mac,IFHWADDRLEN);
    peth->h_proto = htons(ETH_RAW_TEXT);

    return send_addr(snd_buf,sizeof(ethhdr) + len,(struct sockaddr*)&_local_addr,sizeof(struct sockaddr_ll));
}

}//namespace chw