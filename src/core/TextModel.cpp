#include "TextModel.h"
#include "GlobalValue.h"

namespace chw {

TextModel::TextModel(const chw::EventLoop::Ptr& poller) : workmodel(poller)
{
    if(_poller == nullptr)
    {
        _poller = chw::EventLoop::addPoller("TextModel");
    }
    _pTcpServer = nullptr;
    _pTcpClient = nullptr;
    // _pUdpServer = nullptr;
    // _pUdpClient = nullptr;
}

TextModel::~TextModel()
{

}

void TextModel::startmodel()
{
    if(chw::gConfigCmd.role == 's')
    {
        _pTcpServer = std::make_shared<chw::TcpServer>(_poller);
        try {
            _pTcpServer->start<chw::TcpTextSession>(chw::gConfigCmd.server_port);
        } catch(const std::exception &ex) {
            PrintE("ex:%s",ex.what());
            sleep_exit(100*1000);
        }
    }
    else
    {
        _pTcpClient = std::make_shared<chw::TcpTextClient>(_poller);
        _pTcpClient->startConnect(chw::gConfigCmd.server_hostname,chw::gConfigCmd.server_port);
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
                    _pTcpServer->sendclientdata((uint8_t*)msg.c_str(),msg.size());
                }
                else
                {
                    _pTcpClient->senddata((uint8_t*)msg.c_str(),msg.size());
                }
            });
        }

        InfoLNCR << ">";
        usleep(10 * 1000);
    }
}

}//namespace chw 