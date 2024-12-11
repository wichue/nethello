// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "RawFileClient.h"
#include "GlobalValue.h"
#include "config.h"

namespace chw {

RawFileClient::RawFileClient(const chw::EventLoop::Ptr& poller) : RawSocket(poller)
{
    if(chw::gConfigCmd.role == 's')
    {
        _FileTransfer = std::make_shared<chw::FileRecv>();
    }
    else
    {
        _FileTransfer = std::make_shared<chw::FileSend>(poller);
    }

    _FileTransfer->SetSndData(STD_BIND_2(RawFileClient::SendDataToKcpQue,this));

    _KcpClient = std::make_shared<KcpClient>(poller);
    _KcpClient->SetRecvUserCb(STD_BIND_1(RawFileClient::RcvDataFromKcp,this));
}

RawFileClient::~RawFileClient()
{

}

// 接收数据回调（epoll线程执行）
void RawFileClient::onRecv(const Buffer::Ptr &pBuf)
{
    ethhdr* peth = (ethhdr*)pBuf->data();
    uint16_t uEthType = ntohs(peth->h_proto);
    switch(uEthType)
    {
        case ETH_RAW_FILE:
            //把数据交给kcp处理
            _KcpClient->RcvDataToKcp((const char*)pBuf->data() + sizeof(ethhdr) + RAW_FILE_ETH_OFFSET,pBuf->Size() - sizeof(ethhdr) - RAW_FILE_ETH_OFFSET);
        break;

        default:
        break;
    }

    pBuf->Reset();
}

/**
 * @brief 从kcp接收用户数据
 * 
 * @param buf 数据
 */
void RawFileClient::RcvDataFromKcp(const Buffer::Ptr &buf)
{
    _FileTransfer->onRecv(buf);
}

// 错误回调
void RawFileClient::onError(const SockException &ex)
{
    //断开连接事件，一般是EOF
    WarnL << ex.what();
}

/**
 * @brief 开始文件传输，客户端调用
 * 
 */
void RawFileClient::StartFileTransf()
{
    if(_FileTransfer != nullptr)
    {
        _FileTransfer->StartTransf();
    }
}

/**
 * @brief 原始套结字发送数据回调，传递给kcp发送数据，不直接调用
 * 
 * @param buf   [in]数据
 * @param len   [in]数据长度
 * @param kcp   [in]kcp实例
 * @param user  [in]用户句柄
 * @return int  发送成功的数据长度
 */
int RawSendDataCb(const char *buf, int len, ikcpcb *kcp, void *user)
{
    RawFileClient* pClient = (RawFileClient*)user;

    // 加入以太头
    char* snd_buf = (char*)_RAM_NEW_(sizeof(ethhdr) + len);
    memcpy(snd_buf + sizeof(ethhdr),buf,len);

    ethhdr* peth = (ethhdr*)snd_buf;
    memcpy(peth->h_dest,gConfigCmd.dstmac,IFHWADDRLEN);
    memcpy(peth->h_source,pClient->_local_mac,IFHWADDRLEN);
    peth->h_proto = htons(ETH_RAW_FILE);
    
    return pClient->send_addr(snd_buf,sizeof(ethhdr) + len,(struct sockaddr*)&pClient->_local_addr,sizeof(struct sockaddr_ll));
}


/**
 * @brief 创建kcp实例
 * 
 * @param conv 会话标识
 */
void RawFileClient::CreateAKcp(uint32_t conv)
{
    _KcpClient->CreateKcp(conv,RawSendDataCb,this);
    
    std::weak_ptr<RawFileClient> weak_self = std::static_pointer_cast<RawFileClient>(shared_from_this());
    _timer_spd = std::make_shared<Timer>(1, [weak_self]() -> bool {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return false;
        }

        strong_self->onManager();
        return true;
    }, _poller);
}

void RawFileClient::onManager()
{
#if FILE_MODEL_PRINT_SPEED
    uint64_t Rcv_BytesPs = 0;// 接收速率，字节/秒
    double Rcv_speed = 0;// 接收速率
    std::string Rcv_unit = "";// 接收速率单位

    uint64_t Snd_BytesPs = 0;// 发送速率，字节/秒
    double Snd_speed = 0;// 发送速率
    std::string Snd_unit = "";// 发送速率单位

    Rcv_BytesPs = getSock()->getRecvSpeed();
    Snd_BytesPs = getSock()->getSendSpeed();

    speed_human(Snd_BytesPs,Snd_speed,Snd_unit);
    speed_human(Rcv_BytesPs,Rcv_speed,Rcv_unit);
    PrintD("recv speed=%.2f(%s),send speed=%.2f(%s)",Rcv_speed,Rcv_unit.c_str(),Snd_speed,Snd_unit.c_str());
#endif//FILE_MODEL_PRINT_SPEED
}

/**
 * @brief 把数据交给kcp发送，需要发送数据时调用
 * 
 * @param buffer [in]数据
 * @param len    [in]数据长度
 * @return int   成功返回压入队列的数据长度，0则输入为0或缓存满，失败返回小于0
 */
int RawFileClient::SendDataToKcpQue(const char *buffer, int len)
{
    return _KcpClient->KcpSendData(buffer,len);
}

}//namespace chw