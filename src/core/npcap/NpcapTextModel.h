// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __NPCAP_TEXT_CLIENT_H
#define __NPCAP_TEXT_CLIENT_H

#include <stdint.h>
#include "ComProtocol.h"
#include "NpcapSocket.h"

namespace chw {

class NpcapTextModel : public workmodel
{
public:
    using Ptr = std::shared_ptr<NpcapTextModel>;
    NpcapTextModel(const chw::EventLoop::Ptr& poller = nullptr);
    ~NpcapTextModel() override;
    
    virtual void startmodel() override;
    virtual void prepare_exit() override{};

private:
    chw::NpcapSocket::Ptr _pClient;
};

}//namespace chw


#endif//__NPCAP_TEXT_CLIENT_H