// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "RawPressModel.h"
#include "GlobalValue.h"
#include "UdpServer.h"
#include "TcpServer.h"
#include "PressSession.h"
#include "PressClient.h"
#include "MsgInterface.h"
#include "RawPressClient.h"
#include <iomanip>

namespace chw {

RawPressModel::RawPressModel(const chw::EventLoop::Ptr& poller) : workmodel(poller)
{
    if(_poller == nullptr)
    {
        _poller = chw::EventLoop::addPoller("RawPressModel");
    }
    _pClient = nullptr;

    _client_snd_num = 0;
    _client_snd_seq = 0;
    _client_snd_len = 0;

    _server_rcv_num = 0;
    _server_rcv_seq = 0;
    _server_rcv_len = 0;
    _server_rcv_spd = 0;
    _bsending = false;

    _last_lost = 0;
    _last_seq = 0;
}

RawPressModel::~RawPressModel()
{

}

void RawPressModel::startmodel()
{
    _ticker_dur.resetTime();

    _pClient = std::make_shared<chw::RawPressClient>(_poller);
    _pClient->setOnCon([this](const SockException &ex){
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
    
    // 创建定时器，周期打印速率信息到控制台
    double interval = 0;
    chw::gConfigCmd.reporter_interval < 1 ? interval = 1 : interval = chw::gConfigCmd.reporter_interval;
    std::weak_ptr<RawPressModel> weak_self = std::static_pointer_cast<RawPressModel>(shared_from_this());
    _timer = std::make_shared<Timer>(interval, [weak_self]() -> bool {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return false;
        }

        strong_self->onManagerModel();
        return true;
    }, _poller);

    // 主线程
    PrintD("time(s)     send                recv                lost/all(rate)");
    while(true)
    {
        if(gConfigCmd.blksize > 0)
        {
            _bsending = true;
            start_client_press();
        }

        sleep(1);
    }
}

void RawPressModel::prepare_exit()
{
    _bsending = false;
    usleep(100 * 1000);

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

    uint64_t Snd_BytesPs = 0;// 发送速率，字节/秒
    double Snd_speed = 0;// 发送速率
    std::string Snd_unit = "";// 发送速率单位

    uint64_t Rcv_BytesPs = 0;// 接收速率，字节/秒
    double Rcv_speed = 0;// 接收速率
    std::string Rcv_unit = "";// 接收速率单位
    
    Snd_BytesPs = _client_snd_len / uDurTimeS;

    _server_rcv_num = _pClient->GetPktNum();
    _server_rcv_seq = _pClient->GetSeq();
    _server_rcv_len = _pClient->GetRcvLen();
    _server_rcv_spd = _pClient->getSock()->getRecvSpeed();
    Rcv_BytesPs = _server_rcv_len / uDurTimeS;

    speed_human(Snd_BytesPs,Snd_speed,Snd_unit);
    speed_human(Rcv_BytesPs,Rcv_speed,Rcv_unit);

    PrintD("- - - - - - - - - - - - - - - - average- - - - - - - - - -- - - - - - - - -");
    uint32_t lost_num = _server_rcv_seq - _server_rcv_num;// 丢包总数量
    double lost_ratio = 0;// 总的丢包率
    if(_server_rcv_seq != 0)
    {
        lost_ratio = ((double)(lost_num) / (double)_server_rcv_seq) * 100;
    }

    // 输出平均速率
    // PrintD("%-12.0f%-8.2f%-12s%-8.2f%-12s"
    // ,uDurTimeS
    // ,Snd_speed
    // ,Snd_unit.c_str()
    // ,Rcv_speed
    // ,Rcv_unit.c_str());

    InfoL << std::left 
    << std::setw(12) << std::setprecision(0) << std::fixed << uDurTimeS 
    << std::setw(8) << std::setprecision(2) << std::fixed << Snd_speed 
    << std::setw(12) << Snd_unit
    << std::setw(8) << std::setprecision(2) << std::fixed << Rcv_speed
    << std::setw(12) << Rcv_unit;

    // 输出接收和发送包数量,丢包率
    // PrintD("all send pkt:%lu,bytes:%lu; all rcv pkt:%lu,bytes:%lu,seq:%lu,lost:%lu(%.2f%%)"
    // ,_client_snd_num,_client_snd_len
    // ,_server_rcv_num,_server_rcv_len,_server_rcv_seq,lost_num,lost_ratio);

    InfoL 
    << "all send pkt:" << _client_snd_num 
    << ",bytes:" << _client_snd_len
    << "; all rcv pkt:" << _server_rcv_num
    << ",bytes:" << _server_rcv_len
    << ",seq:" << _server_rcv_seq
    << ",lost:" << lost_num
    << "(" << std::setprecision(2) << std::fixed << lost_ratio << "%)";
}

void RawPressModel::onManagerModel()
{
    uint64_t uDurTimeMs = _ticker_dur.elapsedTime();// 当前测试时长ms
    uint64_t uDurTimeS = 0;// 当前测试时长s
    if(uDurTimeMs > 0)
    {
        uDurTimeS = uDurTimeMs / 1000;
    }

    uint64_t Snd_BytesPs = 0;// 发送速率，字节/秒
    double Snd_speed = 0;// 发送速率
    std::string Snd_unit = "";// 发送速率单位

    uint64_t Rcv_BytesPs = 0;// 接收速率，字节/秒
    double Rcv_speed = 0;// 接收速率
    std::string Rcv_unit = "";// 接收速率单位
    
    Snd_BytesPs = _pClient->getSock()->getSendSpeed();

    _server_rcv_num = _pClient->GetPktNum();
    _server_rcv_seq = _pClient->GetSeq();
    _server_rcv_len = _pClient->GetRcvLen();
    _server_rcv_spd = _pClient->getSock()->getRecvSpeed();
    Rcv_BytesPs = _server_rcv_spd;

    speed_human(Snd_BytesPs,Snd_speed,Snd_unit);
    speed_human(Rcv_BytesPs,Rcv_speed,Rcv_unit);

    // 输出速率和丢包率
    uint32_t lost_num = _server_rcv_seq - _server_rcv_num;// 丢包总数量
    uint32_t cur_lost_num = lost_num - _last_lost;// 当前周期丢包数量
    uint32_t cur_rcv_seq = _server_rcv_seq - _last_seq;// 当前周期应该收到包的数量
    double cur_lost_ratio = 0;// 当前周期丢包率
    if(cur_rcv_seq != 0) 
    {
        cur_lost_ratio = ((double)(cur_lost_num) / (double)cur_rcv_seq) * 100;
    }
    // PrintD("%-12u%-8.2f%-12s%-8.2f%-12s%lu/%lu (%.2f%%)"
    // ,uDurTimeS
    // ,Snd_speed
    // ,Snd_unit.c_str()
    // ,Rcv_speed
    // ,Rcv_unit.c_str()
    // ,cur_lost_num
    // ,cur_rcv_seq
    // ,cur_lost_ratio);
    InfoL << std::left 
    << std::setw(12) << std::setprecision(0) << std::fixed << uDurTimeS 
    << std::setw(8) << std::setprecision(2) << std::fixed << Snd_speed 
    << std::setw(12) << Snd_unit
    << std::setw(8) << std::setprecision(2) << std::fixed << Rcv_speed
    << std::setw(12) << Rcv_unit
    << cur_lost_num << "/" << cur_rcv_seq
    << "(" 
    << std::setprecision(2) << std::fixed << cur_lost_ratio << "%)";

    _last_lost = lost_num;
    _last_seq  = _server_rcv_seq;

    if(gConfigCmd.duration > 0 && uDurTimeS >= gConfigCmd.duration)
    {
        prepare_exit();
        sleep_exit(100 * 1000);
    }
}

// raw发送数据[dstmac][srcmac][FF02][MsgHdr][data]
void RawPressModel::start_client_press()
{
    if(gConfigCmd.blksize < 8)
    {
        gConfigCmd.blksize = 8;
    }

    uint32_t buflen = sizeof(ethhdr) + gConfigCmd.blksize;
    char* buf = (char*)_RAM_NEW_(buflen);
    MsgHdr* pMsgHdr = (MsgHdr*)(buf + sizeof(ethhdr));
    pMsgHdr->uMsgIndex = 0;
    pMsgHdr->uTotalLen = gConfigCmd.blksize;

    ethhdr* peth = (ethhdr*)buf;
    memcpy(peth->h_dest,gConfigCmd.dstmac,IFHWADDRLEN);
    memcpy(peth->h_source,_pClient->_local_mac,IFHWADDRLEN);
    peth->h_proto = htons(ETH_RAW_PERF);
    
    // 不控速
    if(gConfigCmd.bandwidth == 0)
    {
        while(_bsending)
        {
            pMsgHdr->uMsgIndex ++;
            uint32_t sndlen = _pClient->send_addr(buf,buflen,(struct sockaddr*)&_pClient->_local_addr,sizeof(struct sockaddr_ll));
            if(sndlen == buflen)
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
        return;
    }

    // 控速
    double uMBps = (double)gConfigCmd.bandwidth;// 每秒需要发送多少MB数据
    uint32_t uByteps = uMBps * 1024 * 1024;// 每秒需要发送多少byte数据
    uint32_t uBytep100ms = uByteps / 10;// 每100ms需要发送多少byte数据
    
    while(_bsending)
    {
        _ticker_ctl.resetTime();
        uint32_t curr_all_sndlen = 0;
        while(1)
        {
            pMsgHdr->uMsgIndex ++;
            uint32_t sndlen = _pClient->send_addr(buf,buflen,(struct sockaddr*)&_pClient->_local_addr,sizeof(struct sockaddr_ll));
            if(sndlen == buflen)
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