// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "NpcapTextModel.h"
#include "GlobalValue.h"

#include <stdint.h>
#include <string>
#include <iostream>

namespace chw {

NpcapTextModel::NpcapTextModel(const chw::EventLoop::Ptr& poller) : workmodel(poller)
{
    if(_poller == nullptr)
    {
        _poller = chw::EventLoop::addPoller("NpcapTextModel");
    }
    _pClient = nullptr;
}

NpcapTextModel::~NpcapTextModel()
{

}

void NpcapTextModel::startmodel()
{
    _pClient = std::make_shared<chw::NpcapSocket>();
    if(_pClient->OpenAdapter(gConfigCmd.interfaceC) == chw::success)
    {
        usleep(1000);
        InfoLNCR << ">";
    }
    else
    {
        sleep_exit(100*1000);
    }

    _pClient->SetReadCb([](const u_char* buf, uint32_t len){
        ethhdr* peth = (ethhdr*)buf;
        uint16_t uEthType = ntohs(peth->h_proto);
        switch(uEthType)
        {
            case ETH_RAW_TEXT:
            {
                //接收数据事件
                PrintD("\b<%s",(char*)buf + sizeof(ethhdr));
                InfoLNCR << ">";
            }
            break;

            default:
            break;
        }
    });

    // 接收在事件循环线程执行
    _poller->async([this](){
        _pClient->StartRecvFromAdapter();
    });
    
    // 发送在主线程执行
    while(true)
    {
        std::string msg;
        std::getline(std::cin, msg);
        if(!msg.empty())
        {
            _pClient->SendToAdapter((char*)msg.c_str(),msg.size());
        }

        InfoLNCR << ">";
        usleep(10 * 1000);
    }
}


}//namespace chw