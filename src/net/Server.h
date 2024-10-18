#ifndef __SERVER_H
#define __SERVER_H

#include <memory>
#include <functional>
#include "Socket.h"
#include "Session.h"

namespace chw {

/**
 * udp和tcp服务端抽象类，实现一些基础功能，成员：Socket::Ptr，EventLoop::Ptr;
 */
class Server : public std::enable_shared_from_this<Server> {
public:
    using Ptr = std::shared_ptr<Server>;
    Server(const EventLoop::Ptr &poller);

    virtual ~Server() = default;

    /**
     * @brief 启动 server
     * 
     * @tparam SessionType 自定义会话类型
     * @param port 本机端口，0则随机
     * @param host 监听网卡ip,"0.0.0.0"默认监听所有ipv4,"::"默认监听所有ipv6
     * @param cb 当有新连接接入时执行的回调，默认nullptr
     */
    template <typename SessionType>
    void start(uint16_t port, const std::string &host = "0.0.0.0", const std::function<void(std::shared_ptr<SessionType> &)> &cb = nullptr)
    {
        static std::string cls_name = chw::demangle(typeid(SessionType).name());
        // Session创建器，通过它创建不同类型的服务器
        _session_alloc = [cb](const Socket::Ptr &sock) {
            auto session = std::shared_ptr<SessionType>(new SessionType(sock), [](SessionType *ptr) {
                delete ptr;
            });
            if (cb) {
                cb(session);
            }
            return std::move(session);
        };
        start_l(port, host);
    }

    /**
     * @brief 返回服务端的Socket
     * 
     * @return const Socket::Ptr& 
     */
    const Socket::Ptr &getSock() const;

    /**
     * @brief 允许自定义socket构建行为,默认使用 Socket::createSocket
     */
    void setOnCreateSocket(Socket::onCreateSocket cb);

    /**
     * @brief 获取服务器监听端口号, 服务器可以选择监听随机端口
     */
    uint16_t getPort();

    /**
     * @brief 发送数据给最后一个活动的客户端
     * 
     * @param buf   数据
     * @param len   数据长度
     * @return uint32_t 发送成功的数据长度
     */
    virtual uint32_t sendclientdata(uint8_t* buf, uint32_t len) = 0;

protected:
    virtual void start_l(uint16_t port, const std::string &host) = 0;
    /**
     * @brief 周期性管理会话，定时器回调
     * 
     */
    virtual void onManagerSession() = 0;

    // socket构建方法
    Socket::onCreateSocket _on_create_socket;

    /**
     * @brief 创建socket
     */
    Socket::Ptr createSocket(const EventLoop::Ptr &poller);

protected:
    Socket::Ptr _socket;
    EventLoop::Ptr _poller;
    std::function<Session::Ptr(const Socket::Ptr &)> _session_alloc;
};

} // namespace chw

#endif//__SERVER_H