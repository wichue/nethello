#include "PressModel.h"
#include "GlobalValue.h"
#include "UdpServer.h"
#include "TcpServer.h"
#include "TextSession.h"
#include "TextClient.h"

namespace chw {

PressModel::PressModel(const chw::EventLoop::Ptr& poller) : workmodel(poller)
{
    if(_poller == nullptr)
    {
        _poller = chw::EventLoop::addPoller("PressModel");
    }
    _pServer = nullptr;
    _pClient = nullptr;
}

PressModel::~PressModel()
{

}

void PressModel::startmodel()
{
    if(chw::gConfigCmd.role == 's')
    {
        if(chw::gConfigCmd.protol == SockNum::Sock_TCP) {
            _pServer = std::make_shared<chw::TcpServer>(_poller);
        } else {
            _pServer = std::make_shared<chw::UdpServer>(_poller);
        }
        
        try {
            _pServer->start<chw::TextSession>(chw::gConfigCmd.server_port);
        } catch(const std::exception &ex) {
            PrintE("ex:%s",ex.what());
            sleep_exit(100*1000);
        }
    }
    else
    {
        if(chw::gConfigCmd.protol == SockNum::Sock_TCP) {
            _pClient = std::make_shared<chw::TcpTextClient>(_poller);
        } else {
            _pClient = std::make_shared<chw::UdpTextClient>(_poller);
        }
        
        _pClient->create_client(chw::gConfigCmd.server_hostname,chw::gConfigCmd.server_port);
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
                    _pServer->sendclientdata((uint8_t*)msg.c_str(),msg.size());
                }
                else
                {
                    _pClient->senddata((uint8_t*)msg.c_str(),msg.size());
                }
            });
        }

        InfoLNCR << ">";
        usleep(10 * 1000);
    }
}

void PressModel::tcp_client_press()
{
    uint8_t* buf = (uint8_t*)_RAM_NEW_(gConfigCmd.blksize);
    // MsgHdr* pMsgHdr = (MsgHdr*)buf;
    
    //控速
    if(gConfigCmd.bandwidth == 0)
    {
        while(1)
        {
            _pClient->senddata(buf,gConfigCmd.blksize);
        }
    }

    double uMBps = (double)gConfigCmd.bandwidth / (double)8;// 每秒需要发送多少MB数据
    uint32_t uByteps = uMBps * 1024 * 1024;// 每秒需要发送多少byte数据
    
}

}//namespace chw 