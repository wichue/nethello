// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "RawFileClient.h"
#include "GlobalValue.h"
#include "config.h"

namespace chw {

RawFileClient::RawFileClient(const chw::EventLoop::Ptr& poller) : RawSocket(poller)
{
    _kcp = nullptr;
    
    if(chw::gConfigCmd.role == 's')
    {
        _FileTransfer = std::make_shared<chw::FileRecv>();
    }
    else
    {
        _FileTransfer = std::make_shared<chw::FileSend>(poller);
    }

    _FileTransfer->SetSndData(STD_BIND_2(RawFileClient::KcpSendData,this));
    _rcv_buf = std::make_shared<Buffer>();
    _rcv_buf->SetCapacity(TCP_BUFFER_SIZE);
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
            KcpRcvData((const char*)pBuf->data() + sizeof(ethhdr) + RAW_FILE_ETH_OFFSET,pBuf->Size() - sizeof(ethhdr) - RAW_FILE_ETH_OFFSET);
        break;

        default:
        break;
    }

    pBuf->Reset();
}

/**
 * @brief kcp处理接收数据
 * 
 * @param buf 数据
 * @param len 数据长度
 */
void RawFileClient::KcpRcvData(const char* buf, long len)
{
    ///6.预接收数据:调用ikcp_input将裸数据交给KCP，这些数据有可能是KCP控制报文
    int ret = ikcp_input(_kcp, buf, len);
    if(ret < 0)//检测ikcp_input是否提取到真正的数据
    {
        return;
    }

    ///7.kcp将接收到的kcp数据包还原成用户数据
    while (1) {
        int nrecv = ikcp_recv(_kcp, (char*)_rcv_buf->data() + _rcv_buf->Size(), _rcv_buf->Idle());
        // printf("ikcp_recv nrecv=%d\n", nrecv);
        if (nrecv < 0) break;
        _rcv_buf->SetSize(nrecv + _rcv_buf->Size());
        _FileTransfer->onRecv(_rcv_buf);
    }

    ///8.每次接收到数据后更新kcp状态(ikcp_update)
    onKcpUpdate();
}

// 错误回调
void RawFileClient::onError(const SockException &ex)
{
    //断开连接事件，一般是EOF
    WarnL << ex.what();
}

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
void RawFileClient::CreateKcp(uint32_t conv)
{
    ///1.创建kcp实例，两端第一个参数conv要相同
    _kcp = ikcp_create(conv, (void *)this);

    ///2.设置kcp发送回调
    _kcp->output = RawSendDataCb;

    ///3.kcp参数设置
    // 配置窗口大小：平均延迟200ms，每20ms发送一个包，
    // 而考虑到丢包重发，设置最大收发窗口为128
    ikcp_wndsize(_kcp, 128, 128);

    // 判断测试用例的模式
    int mode = 2;
    if (mode == 0) {
        // 默认模式
        ikcp_nodelay(_kcp, 0, 10, 0, 0);
    } else if (mode == 1) {
        // 普通模式，关闭流控等
        ikcp_nodelay(_kcp, 0, 10, 0, 1);
    }	else {
        // 启动快速模式
        // 第二个参数 nodelay-启用以后若干常规加速将启动
        // 第三个参数 interval为内部处理时钟，默认设置为 10ms
        // 第四个参数 resend为快速重传指标，设置为2
        // 第五个参数 为是否禁用常规流控，这里禁止
        ikcp_nodelay(_kcp, 2, 10, 2, 1);
        _kcp->rx_minrto = 10;
        _kcp->fastresend = 1;
    }

    ///4.创建定时器周期处理ikcp_update
    std::weak_ptr<RawFileClient> weak_self = std::static_pointer_cast<RawFileClient>(shared_from_this());
    _timer = std::make_shared<Timer>(0.001, [weak_self]() -> bool {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return false;
        }

        ///5.处理kcp发送和重发
        strong_self->onKcpUpdate();
        return true;
    }, _poller);

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
 * @brief 周期执行ikcp_update
 * 
 */
void RawFileClient::onKcpUpdate()
{
    // 传入微秒比毫秒速率快很多
    ikcp_update(_kcp,getCurrentMicrosecond());
}

/**
 * @brief 把数据交给kcp发送，需要发送数据时调用
 * 
 * @param buffer [in]数据
 * @param len    [in]数据长度
 * @return int   成功返回压入队列的数据长度，0则输入为0或缓存满，失败返回小于0
 */
int RawFileClient::KcpSendData(const char *buffer, int len)
{
    //这里只是把数据加入到发送队列
    int ret = ikcp_send(_kcp,buffer,len);
    if(ret >= 0)
    {
        // 通过定时器回调触发 onKcpUpdate 速率更快
        // onKcpUpdate();
    }
    
    return ret;
}

}//namespace chw