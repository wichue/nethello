#ifndef __PRESS_MODEL_H
#define __PRESS_MODEL_H

#include <memory>
#include "ComProtocol.h"
#include "EventLoop.h"
#include "Server.h"
#include "Client.h"

namespace chw {

/**
 *  压力测试模式，客户端发送，服务端接收，统计发送和接收速率，udp统计包数量和丢包率，发送速率可控。
 *  tcp接收数据直接丢弃，udp解析数据头序列号，用于计算包数量和丢包率。
 */
class PressModel : public workmodel
{
public:
    using Ptr = std::shared_ptr<PressModel>;
    PressModel(const chw::EventLoop::Ptr& poller = nullptr);
    ~PressModel() override;

    virtual void startmodel() override;
private:
    void onManagerModel();
    void tcp_client_press();

private:
    chw::Server::Ptr _pServer;
    chw::Client::Ptr _pClient;
    std::shared_ptr<Timer> _timer;
private:
    Ticker _ticker_ctl;// 控速用的计时器
    Ticker _ticker_dur;// 计算测试时长的计时器

    uint64_t _client_snd_num;// 发送包的数量
    uint64_t _client_snd_seq;// 发送包的最大序列号
    uint64_t _client_snd_len;// 发送的字节总大小
public:
    bool _bStart;
};

}//namespace chw 

#endif//__PRESS_MODEL_H