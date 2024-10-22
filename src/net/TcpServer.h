#ifndef __TCP_SERVER_H
#define __TCP_SERVER_H

#include <memory>
#include <functional>
#include <unordered_map>
#include "Server.h"
#include "Session.h"
#include "Timer.h"
#include "util.h"
#include "Socket.h"

namespace chw {

/**
 * tcp服务端，实现监听和新连接接入，管理连接。
 * 接入连接的数据收发在 SessionType 实现。
 */
class TcpServer : public Server {
public:
    using Ptr = std::shared_ptr<TcpServer>;

    explicit TcpServer(const EventLoop::Ptr &poller);
    ~TcpServer();

    /**
     * @brief 获取会话接收信息
     * 
     * @param rcv_num 接收包的数量
     * @param rcv_seq 接收包的最大序列号
     * @param rcv_len 接收的字节总大小
     * @param rcv_speed 接收速率
     */
    virtual void GetRcvInfo(uint64_t& rcv_num,uint64_t& rcv_seq,uint64_t& rcv_len,uint64_t& rcv_speed) override;

protected:
    /**
     * @brief 新接入连接回调（在epoll线程执行）
     * 
     * @param sock  新接入连接Socket
     * @return uint32_t 成功返回chw::success，失败返回chw::fail
     */
    uint32_t onAcceptConnection(const Socket::Ptr &sock);
    Socket::Ptr onBeforeAcceptConnection(const EventLoop::Ptr &poller);

private:
    virtual void onManagerSession() override;

    /**
     * @brief 启动tcp服务端，开始绑定和监听（可在任意线程执行）
     * 
     * @param port 绑定端口
     * @param host 绑定ip
     */
    virtual void start_l(uint16_t port, const std::string &host) override;

    /**
     * @brief 创建Socket，设置新接入连接回调
     * 
     */
    void setupEvent();

private:
    bool _is_on_manager = false;
    std::shared_ptr<Timer> _timer;

//chw
public:
    /**
     * @brief 发送数据给最后一个活动的客户端（可在任意线程执行）
     * 
     * @param buf   数据
     * @param len   数据长度
     * @return uint32_t 发送成功的数据长度
     */
    virtual uint32_t sendclientdata(uint8_t* buf, uint32_t len) override;
    std::weak_ptr<Session> _last_session;// 最后一个活动的客户端
private:
    std::unordered_map<Session *, Session::Ptr> _session_map;
};

} //namespace chw

#endif //__TCP_SERVER_H
