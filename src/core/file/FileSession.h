// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __TCP_TEXT_SESSION_H
#define __TCP_TEXT_SESSION_H

#include <memory>
#include "Session.h"
#include "FileRecv.h"

namespace chw {

/**
 * 文件传输会话，用于服务端接收文件。
 */
class FileSession : public Session {
public:
    using Ptr = std::shared_ptr<FileSession>();
    FileSession(const Socket::Ptr &sock);
    virtual ~FileSession();

    /**
     * @brief 接收数据回调（epoll线程执行）
     * 
     * @param buf [in]数据
     */
    void onRecv(const Buffer::Ptr &buf) override;

    /**
     * @brief 发生错误时的回调
     * 
     * @param err [in]异常
     */
    void onError(const SockException &err) override;

    /**
     * @brief 定时器周期管理回调
     * 
     */
    void onManager() override;

    /**
     * @brief 返回当前类名称
     * 
     * @return const std::string& 类名称
     */
    const std::string &className() const override;

    virtual uint64_t GetPktNum()override{return 0;};
    virtual uint64_t GetSeq()override{return 0;};
    virtual uint64_t GetRcvLen()override{return 0;};

    /**
     * @brief tcp发送数据（可在任意线程执行）
     * 
     * @param buff [in]数据
     * @param len  [in]数据长度
     * @return uint32_t 发送成功的数据长度
     */
    uint32_t senddata(char* buff, uint32_t len) override;
private:
    std::string _cls;// 类名
    FileTransfer::Ptr _FileTransfer;// 文件传输业务
};

}//namespace chw

#endif//__TCP_TEXT_SESSION_H