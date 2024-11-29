// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "NpcapPressModel.h"
#include "GlobalValue.h"
#include "UdpServer.h"
#include "TcpServer.h"
#include "PressSession.h"
#include "PressClient.h"
#include "MsgInterface.h"
#include "config.h"
#include <iomanip>

namespace chw {

NpcapPressModel::NpcapPressModel(const chw::EventLoop::Ptr& poller) : workmodel(poller)
{
    if(_poller == nullptr)
    {
        _poller = chw::EventLoop::addPoller("NpcapPressModel");
    }
    _pClient = nullptr;
    _recv_poller = nullptr;

    _client_snd_num = 0;
    _client_snd_seq = 0;
    _client_snd_len = 0;

    _server_rcv_num = 0;
    _server_rcv_seq = 0;
    _server_rcv_len = 0;
    // _server_rcv_spd = 0;
    _bsending = false;
    _interval = 1;

    _last_lost = 0;
    _last_seq = 0;

    _last_client_snd_len = 0;
    _last_server_rcv_len = 0;
}

NpcapPressModel::~NpcapPressModel()
{

}

void NpcapPressModel::startmodel()
{
    _ticker_dur.resetTime();

    _pClient = std::make_shared<chw::NpcapSocket>();
    if(_pClient->OpenAdapter(gConfigCmd.interfaceC) == chw::fail)
    {
        sleep_exit(100*1000);
    }

    _pClient->SetReadCb([this](const u_char* buf, uint32_t len){
        ethhdr* peth = (ethhdr*)buf;
        uint16_t uEthType = ntohs(peth->h_proto);
        switch(uEthType)
        {
            case ETH_RAW_PERF:
            {
                MsgHdr* pMsgHdr = (MsgHdr*)((char*)buf + sizeof(ethhdr));
                if(_server_rcv_seq < pMsgHdr->uMsgIndex)
                {
                    _server_rcv_seq = pMsgHdr->uMsgIndex;
                }
                
                _server_rcv_num ++;
                _server_rcv_len += len;
            }
            break;

            default:
            break;
        }
    });
    
    // 创建定时器，周期打印速率信息到控制台
    chw::gConfigCmd.reporter_interval < 1 ? _interval = 1 : _interval = chw::gConfigCmd.reporter_interval;
    std::weak_ptr<NpcapPressModel> weak_self = std::static_pointer_cast<NpcapPressModel>(shared_from_this());
    _timer = std::make_shared<Timer>(_interval, [weak_self]() -> bool {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return false;
        }

        strong_self->onManagerModel();
        return true;
    }, _poller);

    // 创建独立线程接收npcap数据，避免阻塞定时器
    if (_recv_poller == nullptr)
    {
        _recv_poller = chw::EventLoop::addPoller("npcap recv poller");
    }

    // 接收在事件循环线程执行
    _recv_poller->async([this]() {
        _pClient->StartRecvFromAdapter();
    },false);

    // 主线程
    PrintD("time(s)     send                recv                lost/all(rate)");
    while(true)
    {
        if(gConfigCmd.blksize > 0)
        {
            _bsending = true;
            start_client_press();
        }

        break;
    }
}

void NpcapPressModel::prepare_exit()
{
    _bsending = false;
    _pClient->StopRecvFromAdapter();
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

    // _server_rcv_num = _pClient->GetPktNum();
    // _server_rcv_seq = _pClient->GetSeq();
    // _server_rcv_len = _pClient->GetRcvLen();
    // _server_rcv_spd = _pClient->getSock()->getRecvSpeed();
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
    PrintD("%-12.0f%-8.2f%-12s%-8.2f%-12s"
    ,uDurTimeS
    ,Snd_speed
    ,Snd_unit.c_str()
    ,Rcv_speed
    ,Rcv_unit.c_str());

    // 输出接收和发送包数量,丢包率
    PrintD("all send pkt:%llu,bytes:%llu; all rcv pkt:%llu,bytes:%llu,seq:%llu,lost:%lu(%.2f%%)"
    ,_client_snd_num,_client_snd_len
    ,_server_rcv_num,_server_rcv_len,_server_rcv_seq,lost_num,lost_ratio);
}

void NpcapPressModel::onManagerModel()
{
    uint64_t uDurTimeMs = _ticker_dur.elapsedTime();// 当前测试时长ms
    double uDurTimeS = 0;// 当前测试时长s
    if(uDurTimeMs > 0)
    {
        uDurTimeS = (double)uDurTimeMs / 1000;
    }

    uint64_t Snd_BytesPs = 0;// 发送速率，字节/秒
    double Snd_speed = 0;// 发送速率
    std::string Snd_unit = "";// 发送速率单位

    uint64_t Rcv_BytesPs = 0;// 接收速率，字节/秒
    double Rcv_speed = 0;// 接收速率
    std::string Rcv_unit = "";// 接收速率单位
    
    uint64_t curr_snd_len = _client_snd_len - _last_client_snd_len;//当前周期发送字节数量
    Snd_BytesPs = curr_snd_len / _interval;
    _last_client_snd_len = _client_snd_len;

    // _server_rcv_num = _pClient->GetPktNum();
    // _server_rcv_seq = _pClient->GetSeq();
    // _server_rcv_len = _pClient->GetRcvLen();
    // _server_rcv_spd = _pClient->getSock()->getRecvSpeed();
    uint64_t curr_rcv_len = _server_rcv_len - _last_server_rcv_len;//当前周期发送字节数量
    Rcv_BytesPs = curr_rcv_len / _interval;
    _last_server_rcv_len = _server_rcv_len;

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
    PrintD("%-12.0f%-8.2f%-12s%-8.2f%-12s%lu/%lu (%.2f%%)"
    ,uDurTimeS
    ,Snd_speed
    ,Snd_unit.c_str()
    ,Rcv_speed
    ,Rcv_unit.c_str()
    ,cur_lost_num
    ,cur_rcv_seq
    ,cur_lost_ratio);

    _last_lost = lost_num;
    _last_seq  = _server_rcv_seq;

    if(gConfigCmd.duration > 0 && uDurTimeS >= gConfigCmd.duration)
    {
        prepare_exit();
        sleep_exit(100 * 1000);
    }
}

// raw发送数据[dstmac][srcmac][FF02][MsgHdr][data]
void NpcapPressModel::start_client_press()
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
    
    uint32_t snd_count = 0;
    // 不控速
    if(gConfigCmd.bandwidth == 0)
    {
        while(_bsending)
        {
            snd_count ++;
            pMsgHdr->uMsgIndex ++;
            // int32_t iRet = _pClient->SendToAdPure(buf,buflen);
            // if(iRet == 0)
            uint32_t uiRet = _pClient->PushNpcapSendQue(buf, buflen);
            if(uiRet == chw::success)
            {
                _client_snd_num ++;
                _client_snd_seq ++;
                _client_snd_len += buflen;
            }
            else
            {
                // 出现错误，退出测试
                prepare_exit();
                sleep_exit(100 * 1000);
            }

            if (snd_count >= NPCAP_SEND_QUEUE_SIZE)
            {
                snd_count = 0;
                _pClient->SendQueToAdPure();
                _pClient->ClearNpcapSendQue();
            }
        }
        _pClient->SendQueToAdPure();
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
            snd_count ++;
            pMsgHdr->uMsgIndex ++;
            // int32_t iRet = _pClient->SendToAdPure(buf,buflen);
            // if(iRet == 0)
            uint32_t uiRet = _pClient->PushNpcapSendQue(buf, buflen);
            if(uiRet == chw::success)
            {
                _client_snd_num ++;
                _client_snd_seq ++;
                _client_snd_len += buflen;
            }
            else
            {
                // 出现错误，退出测试
                prepare_exit();
                sleep_exit(100 * 1000);
            }
            curr_all_sndlen += buflen;

            if (snd_count >= NPCAP_SEND_QUEUE_SIZE)
            {
                snd_count = 0;
                _pClient->SendQueToAdPure();
                _pClient->ClearNpcapSendQue();
            }

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
    _pClient->SendQueToAdPure();
}

}//namespace chw 