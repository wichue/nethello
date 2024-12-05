// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __RAW_FILE_MODEL_H
#define __RAW_FILE_MODEL_H

#include <memory>
#include "ComProtocol.h"
#include "EventLoop.h"
#include "RawFileClient.h"

namespace chw {

/**
 *  文件传输模式，客户端发送，服务端接收。
 *  todo:使用udp结合kcp实现。
 */
class RawFileModel : public workmodel
{
public:
    using Ptr = std::shared_ptr<RawFileModel>;
    RawFileModel(const chw::EventLoop::Ptr& poller = nullptr);
    ~RawFileModel() override;

    /**
     * @brief 启动文件传输模式
     * 
     */
    virtual void startmodel() override;
    virtual void prepare_exit() override{};

private:
    chw::RawFileClient::Ptr _pClient;
};

}//namespace chw 

#endif//__RAW_FILE_MODEL_H