#ifndef __TCP_CLIENT_H
#define __TCP_CLIENT_H

#include <memory>
#include "Socket.h"

namespace chw {

//Tcp客户端，Socket对象默认开始互斥锁
class TcpClient : public std::enable_shared_from_this<TcpClient> {
public:
    using Ptr = std::shared_ptr<TcpClient>;
    TcpClient(const EventLoop::Ptr &poller = nullptr);
    virtual ~TcpClient();

    /**
     * 开始连接tcp服务器
     * @param url 服务器ip或域名
     * @param port 服务器端口
     * @param timeout_sec 超时时间,单位秒
     * @param local_port 本地端口
     */
    virtual void startConnect(const std::string &url, uint16_t port, float timeout_sec = 5, uint16_t local_port = 0);
    
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
     * 设置网卡适配器,使用该网卡与服务器通信
     * @param local_ip 本地网卡ip
     */
    virtual void setNetAdapter(const std::string &local_ip);

    /**
     * 唯一标识
     */
    std::string getIdentifier() const;

protected:
    /**
     * 连接服务器结果回调
     * @param ex 成功与否
     */
    virtual void onConnect(const SockException &ex) = 0;

    /**
     * tcp连接成功后每2秒触发一次该事件
     */
    // void onManager() {}

private:
    void onSockConnect(const SockException &ex);

private:
    mutable std::string _id;
    std::string _net_adapter = "::";
    std::shared_ptr<Timer> _timer;
    //对象个数统计
    // ObjectStatistic<TcpClient> _statistic;

    //chw
public:
    /**
     * 收到 eof 或其他导致脱离 Server 事件的回调
     * 收到该事件时, 该对象一般将立即被销毁
     * @param err 原因
     */
    virtual void onError(const SockException &err) = 0;

    /**
     * 接收数据入口
     * @param buf 数据，可以重复使用内存区,不可被缓存使用
     */
    virtual void onRecv(const Buffer::Ptr &buf) = 0;

    Socket::Ptr _sock;
    EventLoop::Ptr _poller;
};

} //namespace chw
#endif //__TCP_CLIENT_H
