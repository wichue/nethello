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

    /**
     * 创建tcp服务器，listen fd的accept事件会加入到所有的poller线程中监听
     * 在调用TcpServer::start函数时，内部会创建多个子TcpServer对象，
     * 这些子TcpServer对象通过Socket对象克隆的方式在多个poller线程中监听同一个listen fd
     * 这样这个TCP服务器将会通过抢占式accept的方式把客户端均匀的分布到不同的poller线程
     * 通过该方式能实现客户端负载均衡以及提高连接接收速度
     */
    explicit TcpServer(const EventLoop::Ptr &poller);
    ~TcpServer();

    /**
    * @brief 开始tcp server
    * @param port 本机端口，0则随机
    * @param host 监听网卡ip
    */
   
    /**
     * @brief 开始tcp server
     * 
     * @tparam SessionType 
     * @param port 本机端口，0则随机
     * @param host 监听网卡ip
     * @param backlog 指排队等待建立3次握手队列长度
     * @param cb 当有新连接接入时执行的回调，默认nullptr
     */
    // template <typename SessionType>
    // void start(uint16_t port, const std::string &host = "::", uint32_t backlog = 1024, const std::function<void(std::shared_ptr<SessionType> &)> &cb = nullptr) {
    //     static std::string cls_name = chw::demangle(typeid(SessionType).name());
    //     // Session创建器，通过它创建不同类型的服务器
    //     _session_alloc = [cb](const TcpServer::Ptr &server, const Socket::Ptr &sock) {
    //         auto session = std::shared_ptr<SessionType>(new SessionType(sock), [](SessionType *ptr) {
    //             //TraceP(static_cast<Socket *>(ptr->getSock())) << "~" << cls_name;
    //             delete ptr;
    //         });
    //         if (cb) {
    //             cb(session);
    //         }
    //         //TraceP(static_cast<Socket *>(session->getSock())) << cls_name;
    //         // session->setOnCreateSocket(server->_on_create_socket);
    //         // return std::make_shared<SessionHelper>(server, std::move(session), cls_name);
    //         return std::move(session);
    //     };
    //     start_l(port, host, backlog);
    // }

    /**
     * @brief 获取服务器监听端口号, 服务器可以选择监听随机端口
     */
    // uint16_t getPort();//base

    /**
     * @brief 自定义socket构建行为
     */
    // void setOnCreateSocket(Socket::onCreateSocket cb);//base

    /**
     * 根据socket对象创建Session对象
     * 需要确保在socket归属poller线程执行本函数
     */
    // Session::Ptr createSession(const Socket::Ptr &socket);

protected:
    // virtual void cloneFrom(const TcpServer &that);
    // virtual TcpServer::Ptr onCreatServer(const EventLoop::Ptr &poller);

    uint32_t onAcceptConnection(const Socket::Ptr &sock);
    Socket::Ptr onBeforeAcceptConnection(const EventLoop::Ptr &poller);

private:
    virtual void onManagerSession() override;
    // Socket::Ptr createSocket(const EventLoop::Ptr &poller);//base
    virtual void start_l(uint16_t port, const std::string &host) override;
    // Ptr getServer(const EventLoop *) const;
    void setupEvent();

private:
    // bool _multi_poller;
    bool _is_on_manager = false;
    // bool _main_server = true;
    // std::weak_ptr<TcpServer> _parent;
    // Socket::Ptr _socket;
    std::shared_ptr<Timer> _timer;
    Socket::onCreateSocket _on_create_socket;
    // std::unordered_map<SessionHelper *, SessionHelper::Ptr> _session_map;
    // std::function<SessionHelper::Ptr(const TcpServer::Ptr &server, const Socket::Ptr &)> _session_alloc;
    // std::unordered_map<const EventLoop *, Ptr> _cloned_server;
    //对象个数统计  [AUTO-TRANSLATED:3b43e8c2]
    //Object count statistics
    // ObjectStatistic<TcpServer> _statistic;

//chw
public:
    /**
     * @brief 发送数据给最后一个活动的客户端
     * 
     * @param buf   数据
     * @param len   数据长度
     * @return uint32_t 发送成功的数据长度
     */
    virtual uint32_t sendclientdata(uint8_t* buf, uint32_t len) override;
    std::weak_ptr<Session> _last_session;// 最后一个活动的客户端
private:
    // EventLoop::Ptr _poller;
    // std::function<Session::Ptr(const TcpServer::Ptr &server, const Socket::Ptr &)> _session_alloc;
    std::unordered_map<Session *, Session::Ptr> _session_map;
    
};

} //namespace chw

#endif //__TCP_SERVER_H
