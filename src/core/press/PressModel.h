#ifndef __PRESS_MODEL_H
#define __PRESS_MODEL_H

#include <memory>
#include "ComProtocol.h"
#include "EventLoop.h"
#include "Server.h"
#include "Client.h"

namespace chw {

/**
 *  压力测试模式，客户端发送，服务端接收，统计发送和接收速率，udp统计丢包率，发送速率可控。
 *  tcp接收数据直接丢弃，udp解析数据头序列号，用于计算丢包率。
 */
class PressModel : public workmodel
{
public:
    using Ptr = std::shared_ptr<PressModel>;
    PressModel(const chw::EventLoop::Ptr& poller = nullptr);
    ~PressModel() override;

    virtual void startmodel() override;
private:
    void tcp_client_press();

private:
    chw::Server::Ptr _pServer;
    chw::Client::Ptr _pClient;
};

}//namespace chw 

#endif//__PRESS_MODEL_H