// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "RawTextModel.h"
#include "GlobalValue.h"

namespace chw {

RawTextModel::RawTextModel(const chw::EventLoop::Ptr& poller) : workmodel(poller)
{
    if(_poller == nullptr)
    {
        _poller = chw::EventLoop::addPoller("RawTextModel");
    }
    _pClient = nullptr;
}

RawTextModel::~RawTextModel()
{

}

void RawTextModel::startmodel()
{
    _pClient = std::make_shared<chw::RawTextClient>(_poller);
    _pClient->setOnCon([](const SockException &ex){
        if(ex)
        {
            sleep_exit(100*1000);
        }
        else
        {
            usleep(1000);
            InfoLNCR << ">";
        }
    });

    if(gConfigCmd.interfaceC == nullptr) {
        _pClient->create_client("",0);
    } else {
        _pClient->create_client(gConfigCmd.interfaceC,0);
    }
    
    // 主线程
    while(true)
    {
        std::string msg;
        std::getline(std::cin, msg);
        if(!msg.empty())
        {
            _poller->async([this,&msg](){
                _pClient->send_text((char*)msg.c_str(),msg.size());
            });
        }

        InfoLNCR << ">";
        usleep(10 * 1000);
    }
}

}//namespace chw 