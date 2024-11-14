// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "RawPressClient.h"
#include "ComProtocol.h"
#include "MemoryHandle.h"
#include "GlobalValue.h"
#include "MsgInterface.h"

namespace chw {

RawPressClient::RawPressClient(const EventLoop::Ptr &poller) : RawSocket(poller)
{
    _rcv_num = 0;
    _rcv_seq = 0;
    _rcv_len = 0;
}

RawPressClient::~RawPressClient()
{

}

// 接收数据回调（epoll线程执行）
void RawPressClient::onRecv(const Buffer::Ptr &pBuf)
{
    ethhdr* peth = (ethhdr*)pBuf->data();
    uint16_t uEthType = ntohs(peth->h_proto);
    switch(uEthType)
    {
        case ETH_RAW_PERF:
        {
            MsgHdr* pMsgHdr = (MsgHdr*)((char*)pBuf->data() + sizeof(ethhdr));
            if(_rcv_seq < pMsgHdr->uMsgIndex)
            {
                _rcv_seq = pMsgHdr->uMsgIndex;
            }
            
            _rcv_num ++;
            _rcv_len += pBuf->Size();
        }
        break;

        default:
        break;
    }

    pBuf->Reset0();
}

// 错误回调
void RawPressClient::onError(const SockException &ex)
{
    //断开连接事件，一般是EOF
    WarnL << ex.what();
}


/**
 * @brief 返回接收包的数量
 * 
 * @return uint64_t 接收包的数量
 */
uint64_t RawPressClient::GetPktNum()
{
    return _rcv_num;
}

/**
 * @brief 返回接收包的最大序列号
 * 
 * @return uint64_t 接收包的最大序列号
 */
uint64_t RawPressClient::GetSeq()
{
    return _rcv_seq;
}

/**
 * @brief 返回接收字节总大小
 * 
 * @return uint64_t 接收字节总大小
 */
uint64_t RawPressClient::GetRcvLen()
{
    return _rcv_len;
}

}//namespace chw