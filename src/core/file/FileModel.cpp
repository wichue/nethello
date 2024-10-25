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
            _pServer->start<chw::FileSession>(chw::gConfigCmd.server_port);
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
        
        _pClient->create_client(chw::gConfigCmd.server_hostname,chw::gConfigCmd.server_port);
    }
}

}//namespace chw
