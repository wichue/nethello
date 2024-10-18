#ifndef __UDP_SERVER_H
#define __UDP_SERVER_H

// #include "Server.h"
#include <functional>
#include <unordered_map>
#include "Session.h"
#include "Server.h"

namespace chw {

class UdpServer : public Server {
public:
    using Ptr = std::shared_ptr<UdpServer>;
    using PeerIdType = std::string;
    // using onCreateSocket = std::function<Socket::Ptr(const EventLoop::Ptr &)>;

    explicit UdpServer(const EventLoop::Ptr &poller = nullptr);
    ~UdpServer();

    /**
     * @brief 开始监听服务器
     */
    // template<typename SessionType>
    // void start(uint16_t port, const std::string &host = "::", const std::function<void(std::shared_ptr<SessionType> &)> &cb = nullptr) {
    //     static std::string cls_name = chw::demangle(typeid(SessionType).name());
    //     // Session 创建器, 通过它创建不同类型的服务器
    //     _session_alloc = [cb](const UdpServer::Ptr &server, const Socket::Ptr &sock) {
    //         auto session = std::shared_ptr<SessionType>(new SessionType(sock), [](SessionType * ptr) {
    //             // TraceP(static_cast<Session *>(ptr)) << "~" << cls_name;
    //             delete ptr;
    //         });
    //         if (cb) {
    //             cb(session);
    //         }
    //         // TraceP(static_cast<Session *>(session.get())) << cls_name;
    //         // auto sock_creator = server->_on_create_socket;
    //         // session->setOnCreateSocket([sock_creator](const EventLoop::Ptr &poller) {
    //         //     return sock_creator(poller, nullptr, nullptr, 0);
    //         // });
    //         // return std::make_shared<SessionHelper>(server, std::move(session), cls_name);
    //         return std::move(session);
    //     };
    //     start_l(port, host);
    // }

    /**
     * @brief 获取服务器监听端口号, 服务器可以选择监听随机端口
     */
    // uint16_t getPort();

    /**
     * @brief 自定义socket构建行为
     */
    // void setOnCreateSocket(Socket::onCreateSocket cb);

protected:
    // virtual Ptr onCreatServer(const EventLoop::Ptr &poller);
    // virtual void cloneFrom(const UdpServer &that);

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

    /**
     * @brief 创建socket
     */
    // Socket::Ptr createSocket(const EventLoop::Ptr &poller);

    void setupEvent();

private:
    // bool _cloned = false;
    // bool _multi_poller;
    // Socket::Ptr _socket;
    std::shared_ptr<Timer> _timer;
    // Socket::onCreateSocket _on_create_socket;
    //cloned server共享主server的session map，防止数据在不同server间漂移
    std::shared_ptr<std::recursive_mutex> _session_mutex;
    // std::shared_ptr<std::unordered_map<PeerIdType, SessionHelper::Ptr> > _session_map;
    //主server持有cloned server的引用
    // std::unordered_map<EventLoop *, Ptr> _cloned_server;
    // std::function<SessionHelper::Ptr(const UdpServer::Ptr &, const Socket::Ptr &)> _session_alloc;
    // 对象个数统计
    // ObjectStatistic<UdpServer> _statistic;

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
    // std::function<Session::Ptr(const Socket::Ptr &)> _session_alloc;
    std::shared_ptr<std::unordered_map<PeerIdType, Session::Ptr> > _session_map;
};

} // namespace chw

#endif // __UDP_SERVER_H
