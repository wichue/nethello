#ifndef __CLIENT_H
#define __CLIENT_H

#include <memory>
#include <functional>
#include "Socket.h"

namespace chw {

/**
 * udp和tcp客户端抽象基类类，实现一些基础功能，成员：Socket::Ptr，EventLoop::Ptr;
 */
class Client : public std::enable_shared_from_this<Client> {
public:
    using Ptr = std::shared_ptr<Client>;
    Client(const EventLoop::Ptr &poller);
    virtual ~Client() = default;

    virtual uint32_t create_client(const std::string &url, uint16_t port, uint16_t localport = 0,const std::string &localip = "::") = 0;

    /**
     * 主动断开连接
     * @param ex 触发onErr事件时的参数
     */
    void shutdown(const SockException &ex = SockException(Err_shutdown, "self shutdown"));

    /**
     * 连接中或已连接返回true，断开连接时返回false
     */
    virtual bool alive() const;

    /**
     * 唯一标识
     */
    std::string getIdentifier() const;

    /**
     * @brief 发送数据（任意线程执行）
     * 
     * @param buff 数据
     * @param len  数据长度
     * @return uint32_t 发送成功的数据长度
     */
    uint32_t senddata(uint8_t* buff, uint32_t len);

    const Socket::Ptr &getSock() const;

protected:
    /**
     * 派生类的连接服务器结果回调,通常用于tcp
     * @param ex 成功与否
     */
    virtual void onConnect(const SockException &ex) = 0;

    /**
     * 派生类收到 eof 或其他导致脱离 Server 事件的回调
     * 收到该事件时, 该对象一般将立即被销毁
     * @param err 原因
     */
    virtual void onError(const SockException &err) = 0;

    /**
     * 派生类接收数据入口
     * @param buf 数据，可以重复使用内存区,不可被缓存使用
     */
    virtual void onRecv(const Buffer::Ptr &buf) = 0;

protected:
    mutable std::string _id;
    Socket::Ptr _socket;
    EventLoop::Ptr _poller;
};


}//namespace chw 


#endif//__CLIENT_H