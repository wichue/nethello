// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __RAW_TEXT_MODEL_H
#define __RAW_TEXT_MODEL_H

#include <memory>
#include "ComProtocol.h"
#include "EventLoop.h"
#include "RawTextClient.h"

namespace chw {

/**
 *  原始套接字文本模式，RawTextClient之间可以通过命令行><文本聊天，不区分客户端和服务端。
 */
class RawTextModel : public workmodel
{
public:
    using Ptr = std::shared_ptr<RawTextModel>;
    RawTextModel(const chw::EventLoop::Ptr& poller = nullptr);
    ~RawTextModel() override;

    virtual void startmodel() override;
    virtual void prepare_exit() override{};

private:
    chw::RawTextClient::Ptr _pClient;
};

}//namespace chw 

#endif//__RAW_TEXT_MODEL_H