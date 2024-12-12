// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __NPCAP_FILE_MODEL_H
#define __NPCAP_FILE_MODEL_H

#include <memory>
#include "ComProtocol.h"
#include "EventLoop.h"

namespace chw {

class NpcapFileModel : public workmodel
{
public:
    using Ptr = std::shared_ptr<NpcapFileModel>;
    NpcapFileModel(const chw::EventLoop::Ptr& poller = nullptr);
    ~NpcapFileModel() override;

    /**
     * @brief 启动文件传输模式
     * 
     */
    virtual void startmodel() override;
    virtual void prepare_exit() override{};

private:
};

}//namespace chw

#endif//__NPCAP_FILE_MODEL_H