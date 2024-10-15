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
        _pTcpClient->startConnect("127.0.0.1",chw::gConfigCmd.server_port);
    }

    // 主线程
    while(true)
    {
        std::string msg;
        
        std::getline(std::cin, msg);
        if(!msg.empty())
        {
            PrintD("%s",msg.c_str());
            _poller->async([this](){
                // _pTcpClient->se
                // _pTcpClient << "hello";
            });
        }

        InfoLNCR << ">";
        usleep(10 * 1000);
    }
}

}//namespace chw 