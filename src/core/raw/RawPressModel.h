// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __RAW_PRESS_MODEL_H
#define __RAW_PRESS_MODEL_H

#include <memory>
#include "ComProtocol.h"
#include "EventLoop.h"
#include "RawPressClient.h"

namespace chw {

/**
 *  原始套接字压力测试模式，统计发送和接收速率，统计包数量和丢包率。使用-b选项控制发送速率。
 *  原始套接字不区分客户端和服务端，即可发送也可接收，可使用-l选项控制，-l为0则不发送只接收。
 */
class RawPressModel : public workmodel
{
public:
    using Ptr = std::shared_ptr<RawPressModel>;
    RawPressModel(const chw::EventLoop::Ptr& poller = nullptr);
    ~RawPressModel() override;

    virtual void startmodel() override;

    /**
     * @brief 准备退出程序，输出测试总结
     * 
     */
    virtual void prepare_exit() override;
private:
    /**
     * @brief 周期性（-i）输出测试速率等
     * 
     */
    void onManagerModel();

    /**
     * @brief 开始客户端压力测试
     * 
     */
    void start_client_press();

private:
    chw::RawPressClient::Ptr _pClient;
    std::shared_ptr<Timer> _timer;// 周期输出信息定时器(-i选项控制)
    bool _bsending;
private:
    Ticker _ticker_ctl;// 控速用的计时器
    Ticker _ticker_dur;// 计算测试时长的计时器

    // 做为发送端
    volatile uint64_t _client_snd_num;// 发送包的数量
    uint64_t _client_snd_seq;// 发送包的最大序列号
    uint64_t _client_snd_len;// 发送的字节总大小

    // 接收端丢包
    uint32_t _last_lost;// 上次统计时的丢包数量
    uint32_t _last_seq; // 上次统计时的最大序列号

    // 做为接收端
    uint64_t _server_rcv_num;// 接收包的数量
    uint64_t _server_rcv_seq;// 接收包的最大序列号
    uint64_t _server_rcv_len;// 接收的字节总大小
    uint64_t _server_rcv_spd;// 接收速率,单位byte/s
};

}//namespace chw 

#endif//__RAW_PRESS_MODEL_H