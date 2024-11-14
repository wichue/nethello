// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "PressModel.h"
#include "GlobalValue.h"
#include "UdpServer.h"
#include "TcpServer.h"
#include "PressSession.h"
#include "PressClient.h"
#include "MsgInterface.h"
#include <iomanip>

namespace chw {

PressModel::PressModel(const chw::EventLoop::Ptr& poller) : workmodel(poller)
{
    if(_poller == nullptr)
    {
        _poller = chw::EventLoop::addPoller("PressModel");
    }
    _pServer = nullptr;
    _pClient = nullptr;

    _client_snd_num = 0;
    _client_snd_seq = 0;
    _client_snd_len = 0;

    _server_rcv_num = 0;
    _server_rcv_seq = 0;
    _server_rcv_len = 0;
    _server_rcv_spd = 0;
    _bStart = false;
    _rs = "";
}

PressModel::~PressModel()
{

}

void PressModel::startmodel()
{
    _ticker_dur.resetTime();
    
    if(chw::gConfigCmd.role == 's')
    {
        _rs = "recv";
        if(chw::gConfigCmd.protol == SockNum::Sock_TCP) {
            _pServer = std::make_shared<chw::TcpServer>(_poller);
        } else {
            _pServer = std::make_shared<chw::UdpServer>(_poller);
        }
        
        try {
            if(gConfigCmd.bind_address == nullptr) {
                _pServer->start<chw::PressSession>(chw::gConfigCmd.server_port);
            } else {
                _pServer->start<chw::PressSession>(chw::gConfigCmd.server_port,gConfigCmd.bind_address);
            }
        } catch(const std::exception &ex) {
            PrintE("ex:%s",ex.what());
            sleep_exit(100*1000);
        }
    }
    else
    {
        _rs = "send";
        if(chw::gConfigCmd.protol == SockNum::Sock_TCP) {
            _pClient = std::make_shared<chw::PressTcpClient>(_poller);
            _pClient->setOnCon([this](const SockException &ex){
                if(ex)
                {
                    PrintE("tcp connect failed, please check ip and port, ex:%s.", ex.what());
                    sleep_exit(100*1000);
                }
                else
                {
                    PrintD("connect success.");
                    usleep(1000);
                    _bStart = true;
                }
            });
        } else {
            _bStart = true;
            _pClient = std::make_shared<chw::PressUdpClient>(_poller);
        }
        
        if(gConfigCmd.bind_address == nullptr) {
            _pClient->create_client(chw::gConfigCmd.server_hostname,chw::gConfigCmd.server_port,chw::gConfigCmd.client_port);
        } else {
            _pClient->create_client(chw::gConfigCmd.server_hostname,chw::gConfigCmd.server_port,chw::gConfigCmd.client_port,chw::gConfigCmd.bind_address);
        }
        // _pClient->getSock()->SetSndType(Socket::SEND_BUFF);
    }
    
    // 创建定时器，周期打印速率信息到控制台
    double interval = 0;
    chw::gConfigCmd.reporter_interval < 1 ? interval = 1 : interval = chw::gConfigCmd.reporter_interval;
    std::weak_ptr<PressModel> weak_self = std::static_pointer_cast<PressModel>(shared_from_this());
    _timer = std::make_shared<Timer>(interval, [weak_self]() -> bool {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return false;
        }

        strong_self->onManagerModel();
        return true;
    }, _poller);

    // 主线程
    PrintD("time(s)         speed(%s)     ",_rs.c_str());
    while(true)
    {
        if(chw::gConfigCmd.role == 'c')
        {
            start_client_press();
        }
        else
        {
            // 服务端主线程什么都不做
            sleep(1);
        }
    }
}

void PressModel::prepare_exit()
{
    uint32_t uDurTimeMs = _ticker_dur.elapsedTime();// 当前测试时长ms
    double uDurTimeS = 0;// 当前测试时长s
    if(uDurTimeMs > 0)
    {
        uDurTimeS = (double)uDurTimeMs / 1000;
    }
    else
    {
        uDurTimeS = 1;
    }

    if(uDurTimeS == 0)
    {
        uDurTimeS = 1;
    }

    uint64_t BytesPs = 0;//速率，字节/秒
    double speed = 0;// 速率
    std::string unit = "";// 单位

    if(chw::gConfigCmd.role == 's')
    {
        // 做为服务端再获取一次接收数据，防止周期获取的数据不全
        _pServer->GetRcvInfo(_server_rcv_num,_server_rcv_seq,_server_rcv_len,_server_rcv_spd);
        BytesPs = _server_rcv_len / uDurTimeS;
    }
    else
    {
        BytesPs = _client_snd_len / uDurTimeS;
    }
    
    speed_human(BytesPs,speed,unit);

    PrintD("- - - - - - - - - - - - - - - - - - - - - - - - - -- - - - - - - - -");
    if(chw::gConfigCmd.protol == SockNum::Sock_TCP)
    {
        // PrintD("%-16.0f%-8.2f(%s)",uDurTimeS,speed,unit.c_str());
        InfoL << std::left << std::setw(16) << std::setprecision(0) << std::fixed << uDurTimeS << std::setw(8) << std::setprecision(2) << std::fixed << speed << "(" << unit << ")";
    }
    else
    {
        if(chw::gConfigCmd.role == 's')
        {
            uint32_t lost_num = _server_rcv_seq - _server_rcv_num;
            double lost_ratio = ((double)(lost_num) / (double)_server_rcv_seq) * 100;
            // PrintD("%-16.0f%-8.2f(%s)  all %s pkt:%lu,bytes:%lu,seq:%lu,lost:%u(%.2f%%)"
            //     ,uDurTimeS,speed,unit.c_str(),_rs.c_str(),_server_rcv_num,_server_rcv_len,_server_rcv_seq,lost_num,lost_ratio);
            InfoL << std::left << std::setw(16) << std::setprecision(0) << std::fixed << uDurTimeS << std::setw(8) << std::setprecision(2) << std::fixed << speed << "(" << unit << ")"
                << "  all " << _rs << " pkt:" << _server_rcv_num << ",bytes:" << _server_rcv_len
                << ",seq:" << _server_rcv_seq << ",lost:" << lost_num << "(" << std::setprecision(2) << std::fixed << lost_ratio << "%)";
        }
        else
        {
            // PrintD("%-16.0f%-8.2f(%s)  all %s pkt:%lu,bytes:%lu"
            //     ,uDurTimeS,speed,unit.c_str(),_rs.c_str(),_client_snd_num,_client_snd_len);
            InfoL << std::left << std::setw(16) << std::setprecision(0) << std::fixed << uDurTimeS << std::setw(8) << std::setprecision(2) << std::fixed << speed << "(" << unit << ")"
                << "  all " << _rs << " pkt:" << _client_snd_num << ",bytes:" << _client_snd_len;
        }
        
    }
}

void PressModel::onManagerModel()
{
    uint64_t uDurTimeMs = _ticker_dur.elapsedTime();// 当前测试时长ms
    uint64_t uDurTimeS = 0;// 当前测试时长s
    if(uDurTimeMs > 0)
    {
        uDurTimeS = uDurTimeMs / 1000;
    }

    uint64_t BytesPs = 0;//速率，字节/秒
    double speed = 0;// 速率
    std::string unit = "";// 单位
    
    if(chw::gConfigCmd.role == 'c')
    {
        BytesPs = _pClient->getSock()->getSendSpeed();
    }
    else
    {
        _server_rcv_spd = 0;
        _pServer->GetRcvInfo(_server_rcv_num,_server_rcv_seq,_server_rcv_len,_server_rcv_spd);
        BytesPs = _server_rcv_spd;
    }

    speed_human(BytesPs,speed,unit);

    if(chw::gConfigCmd.role == 's' && speed > 0)
    {
        if(chw::gConfigCmd.protol == SockNum::Sock_TCP)
        {
            // PrintD("%-16u%-8.2f(%s)",uDurTimeS,speed,unit.c_str());
            InfoL << std::left << std::setw(16) << uDurTimeS << std::setw(8) << std::setprecision(2) << std::fixed << speed << "(" << unit << ")";
        }
        else
        {
            // udp服务端，输出丢包信息
            uint32_t lost_num = _server_rcv_seq - _server_rcv_num;// 丢包总数量
            uint32_t cur_lost_num = lost_num - _last_lost;// 当前周期丢包数量
            uint32_t cur_rcv_seq = _server_rcv_seq - _last_seq;// 当前周期应该收到包的数量
            double cur_lost_ratio = ((double)(cur_lost_num) / (double)cur_rcv_seq) * 100;// 当前周期丢包率
            PrintD("%-16u%-8.2f(%s)    %u/%u (%.2f%%)",uDurTimeS,speed,unit.c_str(),cur_lost_num,cur_rcv_seq,cur_lost_ratio);

            _last_lost = lost_num;
            _last_seq  = _server_rcv_seq;
        }
    }
    if(chw::gConfigCmd.role == 'c')
    {
        //PrintD("%-16u%-8.2f(%s)",uDurTimeS,speed,unit.c_str());
        InfoL << std::left << std::setw(16) << uDurTimeS << std::setw(8) << std::setprecision(2) << std::fixed << speed << "(" << unit << ")";
    }

    if(gConfigCmd.duration > 0 && uDurTimeS >= gConfigCmd.duration)
    {
        prepare_exit();
        sleep_exit(100 * 1000);
    }
}

// tcp当连接成功后再开始发送数据
void PressModel::start_client_press()
{
    char* buf = (char*)_RAM_NEW_(gConfigCmd.blksize);
    MsgHdr* pMsgHdr = (MsgHdr*)buf;
    pMsgHdr->uMsgIndex = 0;
    pMsgHdr->uTotalLen = gConfigCmd.blksize;
    
    // 不控速
    if(gConfigCmd.bandwidth == 0)
    {
        while(1)
        {
            if(_bStart == false)
            {
                usleep(10 * 1000);
                continue;
            }

            pMsgHdr->uMsgIndex ++;
            uint32_t sndlen = _pClient->senddata_i(buf,gConfigCmd.blksize);
            if(sndlen == gConfigCmd.blksize)
            {
                _client_snd_num ++;
                _client_snd_seq ++;
                _client_snd_len += sndlen;
            }
            else
            {
                // 出现错误，退出测试
                prepare_exit();
                sleep_exit(100 * 1000);
            }
        }
    }

    // 控速
    // todo:实际速率略低于控速速率
    double uMBps = (double)gConfigCmd.bandwidth;// 每秒需要发送多少MB数据
    uint32_t uByteps = uMBps * 1024 * 1024;// 每秒需要发送多少byte数据
    uint32_t uBytep100ms = uByteps / 10;// 每100ms需要发送多少byte数据
    
    while(1)
    {
        if(_bStart == false)
        {
            usleep(10 * 1000);
            continue;
        }

        _ticker_ctl.resetTime();
        uint32_t curr_all_sndlen = 0;
        while(1)
        {
            pMsgHdr->uMsgIndex ++;
            uint32_t sndlen = _pClient->senddata_i(buf,gConfigCmd.blksize);
            if(sndlen == gConfigCmd.blksize)
            {
                _client_snd_num ++;
                _client_snd_seq ++;
                _client_snd_len += sndlen;
            }
            else
            {
                // 出现错误，退出测试
                prepare_exit();
                sleep_exit(100 * 1000);
            }
            curr_all_sndlen += sndlen;

            uint16_t use_ms =  _ticker_ctl.elapsedTime();
            if(use_ms >= 100)
            {
                break;
            }

            if(curr_all_sndlen >= uBytep100ms)
            {
                usleep((100 - use_ms - 1) * 1000);
                break;
            }
        }
    }
}

}//namespace chw 