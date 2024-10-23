#ifndef __PRESS_MODEL_H
#define __PRESS_MODEL_H

#include <memory>
#include "ComProtocol.h"
#include "EventLoop.h"
#include "Server.h"
#include "Client.h"

namespace chw {

/**
 *  压力测试模式，客户端发送，服务端接收，统计发送和接收速率，udp统计包数量和丢包率。
 *  速率控制，客户端带-b选项则客户端控速，服务端带-b选项则服务端控速。
 *  tcp接收数据直接丢弃，udp解析数据头序列号，用于计算包数量和丢包率。
 * 
 *  todo:增加信令通道，测试结束时通知对方
 *  todo:服务端控速
 */
class PressModel : public workmodel
{
public:
    using Ptr = std::shared_ptr<PressModel>;
    PressModel(const chw::EventLoop::Ptr& poller = nullptr);
    ~PressModel() override;

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
    chw::Server::Ptr _pServer;
    chw::Client::Ptr _pClient;
    std::shared_ptr<Timer> _timer;
private:
    Ticker _ticker_ctl;// 控速用的计时器
    Ticker _ticker_dur;// 计算测试时长的计时器
    std::string _rs   ;// 收发角色

    // 做为客户端
    uint64_t _client_snd_num;// 发送包的数量
    uint64_t _client_snd_seq;// 发送包的最大序列号
    uint64_t _client_snd_len;// 发送的字节总大小

    // udp客户端丢包
    uint32_t _last_lost;// 上次统计时的丢包数量
    uint32_t _last_seq; // 上次统计时的最大序列号

    // 做为服务端
    uint64_t _server_rcv_num;// 接收包的数量
    uint64_t _server_rcv_seq;// 接收包的最大序列号
    uint64_t _server_rcv_len;// 接收的字节总大小
    uint64_t _server_rcv_spd;// 接收速率,单位byte
public:
    bool _bStart;
};

}//namespace chw 

#endif//__PRESS_MODEL_H