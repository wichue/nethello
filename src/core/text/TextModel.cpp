// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "TextModel.h"
#include "GlobalValue.h"
#include "UdpServer.h"
#include "TcpServer.h"
#include "TextSession.h"
#include "TextClient.h"

namespace chw {

TextModel::TextModel(const chw::EventLoop::Ptr& poller) : workmodel(poller)
{
    if(_poller == nullptr)
    {
        _poller = chw::EventLoop::addPoller("TextModel");
    }
    _pServer = nullptr;
    _pClient = nullptr;
}

TextModel::~TextModel()
{

}

void TextModel::startmodel()
{
    if(chw::gConfigCmd.role == 's')
    {
        if(chw::gConfigCmd.protol == SockNum::Sock_TCP) {
            _pServer = std::make_shared<chw::TcpServer>(_poller);
        } else {
            _pServer = std::make_shared<chw::UdpServer>(_poller);
        }
        
        try {
            if(gConfigCmd.bind_address == nullptr) {
                _pServer->start<chw::TextSession>(chw::gConfigCmd.server_port);
            } else {
                _pServer->start<chw::TextSession>(chw::gConfigCmd.server_port,gConfigCmd.bind_address);
            }
        } catch(const std::exception &ex) {
            PrintE("ex:%s",ex.what());
            sleep_exit(100*1000);
        }
    }
    else
    {
        if(chw::gConfigCmd.protol == SockNum::Sock_TCP) {
            _pClient = std::make_shared<chw::TcpTextClient>(_poller);
            _pClient->setOnCon([](const SockException &ex){
                if(ex)
                {
                    PrintE("tcp connect failed, please check ip and port, ex:%s.", ex.what());
                    sleep_exit(100*1000);
                }
                else
                {
                    PrintD("connect success.");
                    usleep(1000);
                    InfoLNCR << ">";
                }
            });
        } else {
            _pClient = std::make_shared<chw::UdpTextClient>(_poller);
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
        }
        
        if(gConfigCmd.bind_address == nullptr) {
            _pClient->create_client(chw::gConfigCmd.server_hostname,chw::gConfigCmd.server_port,chw::gConfigCmd.client_port);
        } else {
            _pClient->create_client(chw::gConfigCmd.server_hostname,chw::gConfigCmd.server_port,chw::gConfigCmd.client_port,chw::gConfigCmd.bind_address);
        }
    }

    // 主线程
    while(true)
    {
        std::string msg;
        std::getline(std::cin, msg);
        if(!msg.empty())
        {
            _poller->async([this,&msg](){
                if(chw::gConfigCmd.role == 's')
                {
                    _pServer->sendclientdata((char*)msg.c_str(),msg.size());
                }
                else
                {
                    _pClient->senddata_i((char*)msg.c_str(),msg.size());
                }
            });
        }

        InfoLNCR << ">";
        usleep(10 * 1000);
    }
}

}//namespace chw 