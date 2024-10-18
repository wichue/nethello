#ifndef __TCP_CLIENT_H
#define __TCP_CLIENT_H

#include <memory>
#include "Socket.h"
#include "Client.h"

namespace chw {

/**
 * Tcp客户端基类，实现连接等接口。
 * 继承该类，实现虚函数onConnect、onError、onRecv即可自定义一个tcp客户端。
 */
class TcpClient : public Client {
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
    virtual uint32_t create_client(const std::string &url, uint16_t port, uint16_t localport = 0,const std::string &localip = "::") override;
    
    // /**
    //  * 主动断开连接
    //  * @param ex 触发onErr事件时的参数
    //  */
    // void shutdown(const SockException &ex = SockException(Err_shutdown, "self shutdown"));

    // /**
    //  * 连接中或已连接返回true，断开连接时返回false
    //  */
    // virtual bool alive() const;

    /**
     * 设置网卡适配器,使用该网卡与服务器通信
     * @param local_ip 本地网卡ip
     */
    // virtual void setNetAdapter(const std::string &local_ip);

    /**
     * 唯一标识
     */
    // std::string getIdentifier() const;

protected:
    // /**
    //  * 派生类的连接服务器结果回调
    //  * @param ex 成功与否
    //  */
    // virtual void onConnect(const SockException &ex) = 0;

    // /**
    //  * 派生类收到 eof 或其他导致脱离 Server 事件的回调
    //  * 收到该事件时, 该对象一般将立即被销毁
    //  * @param err 原因
    //  */
    // virtual void onError(const SockException &err) = 0;

    // /**
    //  * 派生类接收数据入口
    //  * @param buf 数据，可以重复使用内存区,不可被缓存使用
    //  */
    // virtual void onRecv(const Buffer::Ptr &buf) = 0;

    /**
     * tcp连接成功后每2秒触发一次该事件
     */
    // void onManager() {}

private:
    /**
     * @brief 连接结果回调
     * 
     * @param ex 
     */
    void onSockConnect(const SockException &ex);

private:
    // mutable std::string _id;
    // std::string _net_adapter = "::";
    // std::shared_ptr<Timer> _timer;
    //对象个数统计
    // ObjectStatistic<TcpClient> _statistic;

    //chw
public:
    /**
     * @brief tcp发送数据
     * 
     * @param buff 数据
     * @param len  数据长度
     * @return uint32_t 发送成功的数据长度
     */
    // uint32_t senddata(uint8_t* buff, uint32_t len);

    // Socket::Ptr _sock;
    // EventLoop::Ptr _poller;
};

} //namespace chw
#endif //__TCP_CLIENT_H
