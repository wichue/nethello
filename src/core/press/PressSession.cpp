// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "PressSession.h"
#include "ComProtocol.h"
#include "MsgInterface.h"

namespace chw {

PressSession::PressSession(const Socket::Ptr &sock) : Session(sock)
{
    _cls = chw::demangle(typeid(PressSession).name());

    _server_rcv_num = 0;
    _server_rcv_seq = 0;
    _server_rcv_len = 0;
}

/**
 * @brief 接收数据回调（epoll线程执行）
 * 
 * @param buf [in]数据
 */
void PressSession::onRecv(const Buffer::Ptr &pBuf)
{
    if(getSock()->sockType() == SockNum::Sock_UDP)
    {
        MsgHdr* pMsgHdr = (MsgHdr*)pBuf->data();
        if(_server_rcv_seq < pMsgHdr->uMsgIndex)
        {
            _server_rcv_seq = pMsgHdr->uMsgIndex;
        }
    }

    _server_rcv_num ++;
    _server_rcv_len += pBuf->RcvLen();

    pBuf->Reset();
}

/**
 * @brief 发生错误时的回调
 * 
 * @param err [in]异常
 */
void PressSession::onError(const SockException &ex)
{
    //断开连接事件，一般是EOF
    WarnL << ex.what();
}

/**
 * @brief 定时器周期管理回调
 * 
 */
void PressSession::onManager()
{

}

/**
 * @brief 返回当前类名称
 * 
 * @return const std::string& 类名称
 */
const std::string &PressSession::className() const 
{
    return _cls;
}

/**
 * @brief 发送数据
 * 
 * @param buff [in]数据
 * @param len  [in]数据长度
 * @return uint32_t 发送成功的数据长度
 */
uint32_t PressSession::senddata(char* buff, uint32_t len)
{
    if(getSock()) {
        return getSock()->send_i(buff,len);
    } else {
        PrintE("tcp session already disconnect");
        return 0;
    }
}

/**
 * @brief 返回接收包的数量
 * 
 * @return uint64_t 接收包的数量
 */
uint64_t PressSession::GetPktNum()
{
    uint64_t tmp = _server_rcv_num;
    _server_rcv_num = 0;
    return tmp;
}

/**
 * @brief 返回接收包的最大序列号
 * 
 * @return uint64_t 接收包的最大序列号
 */
uint64_t PressSession::GetSeq()
{
    return _server_rcv_seq;
}

/**
 * @brief 返回接收字节总大小
 * 
 * @return uint64_t 接收字节总大小
 */
uint64_t PressSession::GetRcvLen()
{
    uint64_t tmp = _server_rcv_len;
    _server_rcv_len = 0;
    return tmp;
}

}//namespace chw