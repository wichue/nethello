// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "FileTcpClient.h"
#include <sys/stat.h>
#include "MsgInterface.h"
#include "ErrorCode.h"
#include "GlobalValue.h"
#include "File.h"
#include "StickyPacket.h"
#include "BackTrace.h"

namespace chw {

FileTcpClient::FileTcpClient(const EventLoop::Ptr &poller) : TcpClient(poller)
{
    _FileTransfer = std::make_shared<chw::FileSend>(poller);
    _FileTransfer->SetSndData(STD_BIND_2(FileTcpClient::SendDataToSer,this));
}

/**
 * @brief 开始文件传输
 * 
 */
void FileTcpClient::StartTransf()
{
    if(_FileTransfer != nullptr)
    {
        _FileTransfer->StartTransf();
    }
}


// 接收数据回调（epoll线程执行）
void FileTcpClient::onRecv(const Buffer::Ptr &pBuf)
{
    _FileTransfer->onRecv(pBuf);
}

// 错误回调
void FileTcpClient::onError(const SockException &ex)
{

}

int FileTcpClient::SendDataToSer(const char *buffer, int len)
{
    return senddata_i((char*)buffer,len);
}

}//namespace chw