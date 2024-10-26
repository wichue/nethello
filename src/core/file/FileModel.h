// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __FILE_MODEL_H
#define __FILE_MODEL_H

#include <memory>
#include "ComProtocol.h"
#include "EventLoop.h"
#include "Server.h"
#include "FileTcpClient.h"

namespace chw {

/**
 *  文件传输模式，客户端发送，服务端接收。
 *  todo:使用udp结合kcp实现。
 */
class FileModel : public workmodel
{
public:
    using Ptr = std::shared_ptr<FileModel>;
    FileModel(const chw::EventLoop::Ptr& poller = nullptr);
    ~FileModel() override;

    /**
     * @brief 启动文件传输模式
     * 
     */
    virtual void startmodel() override;
    virtual void prepare_exit() override{};

private:
    chw::Server::Ptr _pServer;
    chw::FileTcpClient::Ptr _pClient;
};

}//namespace chw 

#endif//__FILE_MODEL_H