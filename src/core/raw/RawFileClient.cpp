// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "RawFileClient.h"
#include "GlobalValue.h"

namespace chw {

RawFileClient::RawFileClient(const chw::EventLoop::Ptr& poller)
{
    _kcp = nullptr;
    _FileSend = std::make_shared<chw::FileSend>(poller);
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
void RawFileClient::onError(const SockException &ex)
{
    //断开连接事件，一般是EOF
    WarnL << ex.what();
}

void RawFileClient::CreateKcp()
{
    ///2.创建kcp实例，两端第一个参数conv要相同
    _kcp = ikcp_create(0x1, (void *)this);

    ///3.kcp参数设置
    // _kcp->output = STD_BIND_4(RawFileClient::raw_sendData,this);//设置udp发送接口
    _RawSndCb = STD_BIND_4(RawFileClient::RawSendDataCb,this);//设置udp发送接口
    _kcp->output = _RawSndCb.target<int(const char *buffer, int len, ikcpcb *kcp, void *user)>();

    // 配置窗口大小：平均延迟200ms，每20ms发送一个包，
    // 而考虑到丢包重发，设置最大收发窗口为128
    ikcp_wndsize(_kcp, 128, 128);

    // 判断测试用例的模式
    int mode = 0;
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

    ///4.启动线程，处理收发
    // m_isLoop = true;
    // pthread_t tid;
    // pthread_create(&tid,NULL,run_udp_thread,this);
}

int RawFileClient::RawSendDataCb(const char *buf, int len, ikcpcb *kcp, void *user)
{
    char* snd_buf = (char*)_RAM_NEW_(sizeof(ethhdr) + len);
    memcpy(snd_buf + sizeof(ethhdr),buf,len);

    ethhdr* peth = (ethhdr*)snd_buf;
    memcpy(peth->h_dest,gConfigCmd.dstmac,IFHWADDRLEN);
    memcpy(peth->h_source,_local_mac,IFHWADDRLEN);
    peth->h_proto = htons(ETH_RAW_TEXT);

    return send_addr(snd_buf,sizeof(ethhdr) + len,(struct sockaddr*)&_local_addr,sizeof(struct sockaddr_ll));
}

int RawFileClient::KcpSendData(const char *buffer, int len)
{
    //这里只是把数据加入到发送队列
    return ikcp_send(_kcp,buffer,len);
}

}//namespace chw