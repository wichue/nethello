// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include"RawFileModel.h"
#include<iostream>
#include "GlobalValue.h"
#include "TcpServer.h"
#include "FileSession.h"
#include "FileTcpClient.h"

namespace chw {

RawFileModel::RawFileModel(const chw::EventLoop::Ptr& poller) : workmodel(poller)
{
    if(_poller == nullptr)
    {
        _poller = chw::EventLoop::addPoller("RawFileModel");
    }
}

RawFileModel::~RawFileModel()
{

}

/**
 * @brief 启动文件传输模式
 * 
 */
void RawFileModel::startmodel()
{
    _pClient = std::make_shared<chw::RawFileClient>(_poller);
    _pClient->setOnCon([](const SockException &ex){
        if(ex)
        {
            sleep_exit(100*1000);
        }
    });

    if(gConfigCmd.interfaceC == nullptr) {
        _pClient->create_client("",0);
    } else {
        _pClient->create_client(gConfigCmd.interfaceC,0);
    }

    _pClient->CreateAKcp(10086);
    if(chw::gConfigCmd.role == 'c')
    {
        _pClient->StartFileTransf();
    }
}

}//namespace chw
