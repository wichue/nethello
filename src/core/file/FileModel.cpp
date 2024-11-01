// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include"FileModel.h"
#include<iostream>
#include "GlobalValue.h"
#include "TcpServer.h"
#include "FileSession.h"
#include "FileTcpClient.h"

namespace chw {

FileModel::FileModel(const chw::EventLoop::Ptr& poller) : workmodel(poller)
{
    if(_poller == nullptr)
    {
        _poller = chw::EventLoop::addPoller("FileModel");
    }
    _pServer = nullptr;
    _pClient = nullptr;
}

FileModel::~FileModel()
{

}

/**
 * @brief 启动文件传输模式
 * 
 */
void FileModel::startmodel()
{
    if(chw::gConfigCmd.role == 's')
    {
        _pServer = std::make_shared<chw::TcpServer>(_poller);
        
        try {
            if(gConfigCmd.bind_address == nullptr) {
                _pServer->start<chw::FileSession>(chw::gConfigCmd.server_port);
            } else {
                _pServer->start<chw::FileSession>(chw::gConfigCmd.server_port,gConfigCmd.bind_address);
            }
        } catch(const std::exception &ex) {
            PrintE("ex:%s",ex.what());
            sleep_exit(100*1000);
        }
    }
    else
    {
        _pClient = std::make_shared<chw::FileTcpClient>(_poller);
        _pClient->setOnCon([this](const SockException &ex){
            if(ex)
            {
                PrintE("tcp connect failed, please check ip and port, ex:%s.", ex.what());
                sleep_exit(100*1000);
            }
            else
            {
                _pClient->StartTransf();
            }
        });
        
        if(gConfigCmd.bind_address == nullptr) {
            _pClient->create_client(chw::gConfigCmd.server_hostname,chw::gConfigCmd.server_port,chw::gConfigCmd.client_port);
        } else {
            _pClient->create_client(chw::gConfigCmd.server_hostname,chw::gConfigCmd.server_port,chw::gConfigCmd.client_port,chw::gConfigCmd.bind_address);
        }
    }
}

}//namespace chw
