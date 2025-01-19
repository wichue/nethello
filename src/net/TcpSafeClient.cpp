// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "TcpSafeClient.h"

namespace chw {

TcpSafeClient::TcpSafeClient(const EventLoop::Ptr &poller) : TcpClient(poller)
{
    _poller = poller;
    _bVaildSnd = false;
    _fd = -1;

    setOnCon([&](const SockException &ex){
        if(ex)
        {
            PrintE("tcp connect failed, please check ip and port, ex:%s.", ex.what());
            sleep_exit(100*1000);
        }
        else
        {
            _bVaildSnd = true;
            _fd = getSock()->rawFD();
            PrintD("connect success.");
        }
    });
}
  
TcpSafeClient::~TcpSafeClient() {

}

void TcpSafeClient::onRecv(const Buffer::Ptr &pBuf)
{
    if(_fd != getSock()->rawFD())
    {
        return;
    }

    //todo,读取数据
}

void TcpSafeClient::onError(const SockException &ex)
{
    std::lock_guard<std::mutex> lck(_mtx);

    if(_fd != getSock()->rawFD())
    {
        return;
    }

    if(_fd > 0)
    {
        //todo:释放链接
    }

    //执行一些初始化
    _bVaildSnd = false;
    _fd = -1;
}

int32_t TcpSafeClient::SendData(uint8_t *pData, uint32_t uiLen, uint32_t ip, uint16_t port, bool checkDst)
{
    std::lock_guard<std::mutex> lck(_mtx);

    // socket还没有创建
    if(_fd < 0)
    {
        // 同步创建socket
    }

    // socket未就绪
    if(_bVaildSnd == false)
    {
        return 0;
    }

    if(checkDst == true)
    {
        //检测目标ip和port变化，则断开连接重连
    }

    uint32_t ret = senddata_i((char*)pData,uiLen);
    if(ret == 0)
    {
        //发送失败
        _bVaildSnd = false;//同步执行，防止业务线程还用该socket发送数据

        _poller->async([this](){
            std::lock_guard<std::mutex> lck(_mtx);
            if(_fd > 0)
            {
                _fd = -1;
            }
        });
    }

    return ret;
}

void TcpSafeClient::DealSocketError(int32_t fd)
{
    std::lock_guard<std::mutex> lck(_mtx);

    // 可能在异步关闭时又重新创建了fd，和原epoll的fd不一致
    if(fd != _fd)
    {
        return;
    }

    if(_fd > 0)
    {
        // 释放链路
        _fd = -1;
    }
}

}//namespace chw 