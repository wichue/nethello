// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __FILE_TRANSFER_H
#define __FILE_TRANSFER_H

#include <memory>
#include "Buffer.h"
#include "Socket.h"

namespace chw {

// 定义文件传输基类
class FileTransfer : public std::enable_shared_from_this<FileTransfer> {
public:
    using Ptr = std::shared_ptr<FileTransfer>;
    FileTransfer() = default;
    virtual ~FileTransfer() = default;

    // 接收数据回调
    virtual void onRecv(const Buffer::Ptr &pBuf) = 0;

    // 错误回调
    virtual void onError(const SockException &ex) = 0;

    // 设置发送回调
    virtual void SetSndData(std::function<uint32_t(char* buf, uint32_t )> cb) = 0;

    // 开始文件传输
    virtual void StartTransf() = 0;
private:
    // 接收消息分发
    virtual void DispatchMsg(char* buf, uint32_t len) = 0;
protected:
    uint32_t _status;// 文件传输状态，参考 FileTranStatus

    /**
     * @brief 发送数据
     * 
     * @param buff [in]数据
     * @param len  [in]数据长度
     * @return uint32_t 发送成功的数据长度
     */
    std::function<uint32_t(char*, uint32_t )> _send_data;
};

}//namespace chw


#endif//__FILE_TRANSFER_H