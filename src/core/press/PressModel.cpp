#include "PressModel.h"
#include "GlobalValue.h"
#include "UdpServer.h"
#include "TcpServer.h"
#include "PressSession.h"
#include "PressClient.h"

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
    _bStart = false;
}

PressModel::~PressModel()
{

}

void PressModel::startmodel()
{
    _ticker_dur.resetTime();
    std::string rs = "";// 收发角色
    if(chw::gConfigCmd.role == 's')
    {
        rs = "recv";
        if(chw::gConfigCmd.protol == SockNum::Sock_TCP) {
            _pServer = std::make_shared<chw::TcpServer>(_poller);
        } else {
            _pServer = std::make_shared<chw::UdpServer>(_poller);
        }
        
        try {
            _pServer->start<chw::PressSession>(chw::gConfigCmd.server_port);
        } catch(const std::exception &ex) {
            PrintE("ex:%s",ex.what());
            sleep_exit(100*1000);
        }
    }
    else
    {
        rs = "send";
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
            _pClient = std::make_shared<chw::PressUdpClient>(_poller);
        }
        
        _pClient->create_client(chw::gConfigCmd.server_hostname,chw::gConfigCmd.server_port);
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
    PrintD("time(s) speed(%s)",rs.c_str());
    while(true)
    {
        if(chw::gConfigCmd.role == 'c')
        {
            tcp_client_press();
        }
        else
        {
            // 服务端主线程什么都不做
            sleep(1);
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
        uint64_t _server_rcv_num = 0;// 接收包的数量
        uint64_t _server_rcv_seq = 0;// 接收包的最大序列号
        uint64_t _server_rcv_len = 0;// 接收的字节总大小
        uint64_t _server_rcv_spd = 0;// 接收速率,单位byte

        _pServer->GetRcvInfo(_server_rcv_num,_server_rcv_seq,_server_rcv_len,_server_rcv_spd);
        BytesPs = _server_rcv_spd;
    }

    if(BytesPs < 1024)
    {
        speed = BytesPs;
        unit = "Bytes/s";
    }
    else if(BytesPs >= 1024 && BytesPs < 1024 * 1024)
    {
        speed = (double)BytesPs / 1024;
        unit = "KB/s";
    }
    else if(BytesPs >= 1024 * 1024 && BytesPs < 1024 * 1024 * 1024)
    {
        speed = (double)BytesPs / 1024 / 1024;
        unit = "MB/s";
    }
    else
    {
        speed = (double)BytesPs / 1024 / 1024 / 1024;
        unit = "GB/s";
    }

    if(chw::gConfigCmd.role == 'c' || (chw::gConfigCmd.role == 's' && speed > 0))
    {
        PrintD("%-8u%-8.2f(%s)",uDurTimeS,speed,unit.c_str());
    }

    if(gConfigCmd.duration > 0 && uDurTimeS >= gConfigCmd.duration)
    {
        //todo:输出测试总结，退出测试
    }
}

// tcp当连接成功后再开始发送数据
void PressModel::tcp_client_press()
{
    uint8_t* buf = (uint8_t*)_RAM_NEW_(gConfigCmd.blksize);
    
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

            uint32_t sndlen = _pClient->senddata(buf,gConfigCmd.blksize);
            if(sndlen == gConfigCmd.blksize)
            {
                _client_snd_num ++;
                _client_snd_seq ++;
                _client_snd_len += sndlen;
            }
        }
    }

    // 控速
    double uMBps = (double)gConfigCmd.bandwidth / (double)8;// 每秒需要发送多少MB数据
    uint32_t uByteps = uMBps * 1024 * 1024;// 每秒需要发送多少byte数据
    uint32_t uBytep100ms = uByteps * 10;// 每100ms需要发送多少byte数据
    
    while(1)
    {
        if(_bStart == false)
        {
            usleep(10 * 1000);
            continue;
        }

        _ticker_ctl.resetTime();
        while(1)
        {
            uint32_t curr_all_sndlen = 0;
            uint32_t sndlen = _pClient->senddata(buf,gConfigCmd.blksize);
            if(sndlen == gConfigCmd.blksize)
            {
                _client_snd_num ++;
                _client_snd_seq ++;
                _client_snd_len += sndlen;
            }
            curr_all_sndlen += sndlen;

            uint16_t use_ms =  _ticker_ctl.elapsedTime();
            if(use_ms >= 100)
            {
                break;
            }

            if(curr_all_sndlen >= uBytep100ms)
            {
                usleep((100 - use_ms) * 1000);
                break;
            }
        }
    }
}

}//namespace chw 