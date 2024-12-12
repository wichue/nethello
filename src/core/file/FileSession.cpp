// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "FileSession.h"
#include "ComProtocol.h"
#include "MsgInterface.h"
#include "ErrorCode.h"
#include "File.h"
#include "StickyPacket.h"

namespace chw {

FileSession::FileSession(const Socket::Ptr &sock) :Session(sock)
{
    _cls = chw::demangle(typeid(FileSession).name());

    _FileTransfer = std::make_shared<chw::FileRecv>();
    _FileTransfer->SetSndData(STD_BIND_2(FileSession::senddata,this));
}

FileSession::~FileSession()
{

}

/**
 * @brief 接收数据回调（epoll线程执行）
 * 
 * @param buf [in]数据
 */
void FileSession::onRecv(const Buffer::Ptr &buf)
{
    _FileTransfer->onRecv(buf);
}

/**
 * @brief 发生错误时的回调
 * 
 * @param err 异常
 */
void FileSession::onError(const SockException &err)
{

}

/**
 * @brief 定时器周期管理回调
 * 
 */
void FileSession::onManager()
{

}

/**
 * @brief 返回当前类名称
 * 
 * @return const std::string& 类名称
 */
const std::string &FileSession::className() const 
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
uint32_t FileSession::senddata(char* buff, uint32_t len)
{
    if(getSock()) {
        return getSock()->send_i(buff,len);
    } else {
        PrintE("tcp session already disconnect");
        return 0;
    }
}

}//namespace chw