#ifndef __UDP_SERVER_H
#define __UDP_SERVER_H

#include <functional>
#include <unordered_map>
#include "Session.h"
#include "Server.h"

namespace chw {

class UdpServer : public Server {
public:
    using Ptr = std::shared_ptr<UdpServer>;
    using PeerIdType = std::string;

    explicit UdpServer(const EventLoop::Ptr &poller = nullptr);
    ~UdpServer();

private:
    /**
     * @brief 开始udp server
     * @param port 本机端口，0则随机
     * @param host 监听网卡ip
     */
    virtual void start_l(uint16_t port, const std::string &host = "::") override;

    /**
     * @brief 定时管理 Session, UDP 会话需要根据需要处理超时
     */
    virtual void onManagerSession() override;

    void onRead(Buffer::Ptr &buf, struct sockaddr *addr, int addr_len);

    /**
     * @brief 接收到数据,可能来自server fd，也可能来自peer fd
     * @param is_server_fd 时候为server fd
     * @param id 客户端id
     * @param buf 数据
     * @param addr 客户端地址
     * @param addr_len 客户端地址长度
     */
    void onRead_l(bool is_server_fd, const PeerIdType &id, Buffer::Ptr &buf, struct sockaddr *addr, int addr_len);

    /**
     * @brief 根据对端信息获取或创建一个会话
     */
    Session::Ptr getOrCreateSession(const PeerIdType &id, Buffer::Ptr &buf, struct sockaddr *addr, int addr_len, bool &is_new);

    /**
     * @brief 创建一个会话, 同时进行必要的设置
     */
    Session::Ptr createSession(const PeerIdType &id, Buffer::Ptr &buf, struct sockaddr *addr, int addr_len);

    void setupEvent();

private:
    std::shared_ptr<Timer> _timer;
    std::shared_ptr<std::recursive_mutex> _session_mutex;

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
    std::shared_ptr<std::unordered_map<PeerIdType, Session::Ptr> > _session_map;
};

} // namespace chw

#endif // __UDP_SERVER_H
