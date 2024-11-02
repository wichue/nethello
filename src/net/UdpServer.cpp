// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "uv_errno.h"
#include "onceToken.h"
#include "UdpServer.h"

using namespace std;

namespace chw {

static const uint8_t s_in6_addr_maped[]
    = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 };

static constexpr auto kUdpDelayCloseMS = 3 * 1000;

/**
 * @brief 根据网络地址，返回唯一标识std::string
 * 
 * @param addr 网络地址
 * @return UdpServer::PeerIdType 唯一标识std::string
 */
static UdpServer::PeerIdType makeSockId(sockaddr *addr, int) {
    UdpServer::PeerIdType ret;
    switch (addr->sa_family) {
        case AF_INET : {
            ret.resize(18);
            ret[0] = ((struct sockaddr_in *) addr)->sin_port >> 8;
            ret[1] = ((struct sockaddr_in *) addr)->sin_port & 0xFF;
            //ipv4地址统一转换为ipv6方式处理
            memcpy(&ret[2], &s_in6_addr_maped, 12);
            memcpy(&ret[14], &(((struct sockaddr_in *) addr)->sin_addr), 4);
            return ret;
        }
        case AF_INET6 : {
            ret.resize(18);
            ret[0] = ((struct sockaddr_in6 *) addr)->sin6_port >> 8;
            ret[1] = ((struct sockaddr_in6 *) addr)->sin6_port & 0xFF;
            memcpy(&ret[2], &(((struct sockaddr_in6 *)addr)->sin6_addr), 16);
            return ret;
        }
        default: assert(0); return "";
    }
}

UdpServer::UdpServer(const EventLoop::Ptr &poller) : Server(poller) {
    setOnCreateSocket(nullptr);
}

/**
 * @brief 创建服务端Socket，设置接收回调
 * 
 */
void UdpServer::setupEvent() {
    _socket = createSocket(_poller);
    std::weak_ptr<UdpServer> weak_self = std::static_pointer_cast<UdpServer>(shared_from_this());
    _socket->setOnRead([weak_self](Buffer::Ptr &buf, struct sockaddr *addr, int addr_len) {
        if (auto strong_self = weak_self.lock()) {
            strong_self->onRead(buf, addr, addr_len);
        }
    });
}

UdpServer::~UdpServer() {
    InfoL << "Close udp server [" << _socket->get_local_ip() << "]: " << _socket->get_local_port();

    _timer.reset();
    _socket.reset();
    _session_map->clear();
}

/**
 * @brief 开始udp server
 * @param port [in]本机端口，0则随机
 * @param host [in]监听网卡ip
 */
void UdpServer::start_l(uint16_t port, const std::string &host) {
    setupEvent();
    //主server才创建session map，其他cloned server共享之
    _session_mutex = std::make_shared<std::recursive_mutex>();
    _session_map = std::make_shared<std::unordered_map<PeerIdType, Session::Ptr> >();

    // 新建一个定时器定时管理这些 udp 会话
    std::weak_ptr<UdpServer> weak_self = std::static_pointer_cast<UdpServer>(shared_from_this());
    _timer = std::make_shared<Timer>(2.0f, [weak_self]() -> bool {
        if (auto strong_self = weak_self.lock()) {
            strong_self->onManagerSession();
            return true;
        }
        return false;
    }, _poller);

    if (!_socket->bindUdpSock(port, host.c_str())) {
        // udp 绑定端口失败, 可能是由于端口占用或权限问题
        std::string err = (StrPrinter << "Bind udp socket on " << host << " " << port << " failed: " << get_uv_errmsg(true));
        throw std::runtime_error(err);
    }

    InfoL << "UDP server bind to [" << host << "]: " << port << ",fd:" << _socket->rawFD();
}

/**
 * @brief udp服务端Socket接收回调
 * 
 * @param buf       [in]数据
 * @param addr      [in]对端地址
 * @param addr_len  [in]对端地址长度
 */
void UdpServer::onRead(Buffer::Ptr &buf, sockaddr *addr, int addr_len) {
    const auto id = makeSockId((struct sockaddr *)addr, addr_len);
    onRead_l(true, id, buf, addr, addr_len);
}

/**
 * @brief udp会话接收消息回调
 * 
 * @param session [in]会话
 * @param buf     [in]数据
 */
static void emitSessionRecv(const Session::Ptr &session, const Buffer::Ptr &buf) {
    if (!session->enable) {
        // 延时销毁中
        return;
    }
    try {
        session->onRecv(buf);
    } catch (SockException &ex) {
        session->getSock()->shutdown(ex);
    } catch (exception &ex) {
        session->getSock()->shutdown(SockException(Err_shutdown, ex.what()));
    }
}

/**
 * @brief udp服务端Socket接收到数据,获取或创建session,消息可能来自server fd，也可能来自peer fd
 * @param is_server_fd  [in]是否为server fd
 * @param id            [in]客户端id
 * @param buf           [in]数据
 * @param addr          [in]客户端地址
 * @param addr_len      [in]客户端地址长度
 */
void UdpServer::onRead_l(bool is_server_fd, const UdpServer::PeerIdType &id, Buffer::Ptr &buf, sockaddr *addr, int addr_len) {
    // udp server fd收到数据时触发此函数；大部分情况下数据应该在peer fd触发，此函数应该不是热点函数
    bool is_new = false;
    if (auto helper = getOrCreateSession(id, buf, addr, addr_len, is_new)) {
        //当前线程收到数据，直接处理数据
        emitSessionRecv(helper, buf);
        _last_session = helper;

#if !defined(NDEBUG)
        if (!is_new) {
            // TraceL << "UDP packet incoming from " << (is_server_fd ? "server fd" : "other peer fd");//chw
        }
#endif
    }
}

/**
 * @brief 定时管理 Session, UDP 会话需要根据需要处理超时
 */
void UdpServer::onManagerSession() {
    decltype(_session_map) copy_map;
    {
        std::lock_guard<std::recursive_mutex> lock(*_session_mutex);
        //拷贝map，防止遍历时移除对象
        copy_map = std::make_shared<std::unordered_map<PeerIdType, Session::Ptr> >(*_session_map);
    }
    auto lam = [copy_map]() {
        for (auto &pr : *copy_map) {
            auto &session = pr.second;
            try {
                // UDP 会话需要处理超时
                session->onManager();
            } catch (exception &ex) {
                WarnL << "Exception occurred when emit onManager: " << ex.what();
            }
        }
    };
    
    lam();
}

/**
 * @brief 根据对端信息获取或创建一个会话
 * 
 * @param id        [in]会话唯一标识
 * @param buf       [in]buf
 * @param addr      [in]对端地址
 * @param addr_len  [in]对端地址长度
 * @param is_new    [out]是否新接入连接
 * @return Session::Ptr 会话
 */
Session::Ptr UdpServer::getOrCreateSession(const UdpServer::PeerIdType &id, Buffer::Ptr &buf, sockaddr *addr, int addr_len, bool &is_new) {
    {
        //减小临界区
        std::lock_guard<std::recursive_mutex> lock(*_session_mutex);
        auto it = _session_map->find(id);
        if (it != _session_map->end()) {
            return it->second;
        }
    }
    is_new = true;
    return createSession(id, buf, addr, addr_len);
}

/**
 * @brief 创建一个会话, 同时进行必要的设置
 * 
 * @param id        [in]会话唯一标识
 * @param buf       [in]buf
 * @param addr      [in]对端地址
 * @param addr_len  [in]对端地址长度
 * @return Session::Ptr 会话
 */
Session::Ptr UdpServer::createSession(const PeerIdType &id, Buffer::Ptr &buf, struct sockaddr *addr, int addr_len) {
    // 此处改成自定义获取poller对象，防止负载不均衡
    auto socket = createSocket(_poller);
    if (!socket) {
        //创建socket失败，本次onRead事件收到的数据直接丢弃
        return nullptr;
    }

    auto addr_str = string((char *) addr, addr_len);
    std::weak_ptr<UdpServer> weak_self = std::static_pointer_cast<UdpServer>(shared_from_this());
    auto helper_creator = [this, weak_self, socket, addr_str, id]() -> Session::Ptr {
        auto server = weak_self.lock();
        if (!server) {
            return nullptr;
        }

        //如果已经创建该客户端对应的UdpSession类，那么直接返回
        lock_guard<std::recursive_mutex> lck(*_session_mutex);
        auto it = _session_map->find(id);
        if (it != _session_map->end()) {
            return it->second;
        }

        assert(_socket);
        socket->bindUdpSock(_socket->get_local_port(), _socket->get_local_ip());
        //chw:服务端必须使用硬绑定,否则内核无法正确把消息分发到各个接入的session
        socket->bindPeerAddr((struct sockaddr *)addr_str.data(), sizeof(struct sockaddr));

        PrintD("create udp session ,local ip=%s,local port=%d,peer ip=%s,peer port=%d,fd=%d"
        ,socket->get_local_ip().c_str(),socket->get_local_port(),socket->get_peer_ip().c_str(),socket->get_peer_port(),socket->rawFD());
        

        auto session = _session_alloc(socket);
        // 把本服务器的配置传递给 Session
        // helper->session()->attachServer(*this);

        std::weak_ptr<Session> weak_session = session;
        socket->setOnRead([weak_self, weak_session, id](Buffer::Ptr &buf, struct sockaddr *addr, int addr_len) {
            auto strong_self = weak_self.lock();
            if (!strong_self) {
                return;
            }

            //快速判断是否为本会话的的数据, 通常应该成立
            if (id == makeSockId((struct sockaddr *)addr, addr_len)) 
            {
                if (auto strong_session = weak_session.lock()) {
                    emitSessionRecv(strong_session, buf);
                    strong_self->_last_session = strong_session;
                }
                else 
                    PrintD("error");
                return;
            }
            //todo:id不同
            else
            {
                PrintD("error");
            }

            //收到非本peer fd的数据，让server去派发此数据到合适的session对象
        });
        socket->setOnErr([weak_self, weak_session, id](const SockException &err) {
            // 在本函数作用域结束时移除会话对象
            // 目的是确保移除会话前执行其 onError 函数
            // 同时避免其 onError 函数抛异常时没有移除会话对象
            onceToken token(nullptr, [&]() {
                // 移除掉会话  [AUTO-TRANSLATED:1d786335]
                //Remove the session
                auto strong_self = weak_self.lock();
                if (!strong_self) {
                    return;
                }
                // 延时移除udp session, 防止频繁快速重建对象
                strong_self->_poller->doDelayTask(kUdpDelayCloseMS, [weak_self, id]() {
                    if (auto strong_self = weak_self.lock()) {
                        // 从共享map中移除本session对象
                        lock_guard<std::recursive_mutex> lck(*strong_self->_session_mutex);
                        strong_self->_session_map->erase(id);
                    }
                    return 0;
                });
            });

            // 获取会话强应用
            if (auto strong_helper = weak_session.lock()) {
                // 触发 onError 事件回调
                TraceP(strong_helper->getSock()) << strong_helper->className() << " on err: " << err;
                strong_helper->enable = false;
                strong_helper->onError(err);
            }
        });
        
        auto pr = _session_map->emplace(id, std::move(session));
        assert(pr.second);
        return pr.first->second;
    };

    if (socket->getPoller()->isCurrentThread()) {
        // 该socket分配在本线程，直接创建helper对象
        return helper_creator();
    }
    PrintD("error");

    return nullptr;
}

/**
 * @brief 发送数据给最后一个活动的客户端
 * 
 * @param buf   [in]数据
 * @param len   [in]数据长度
 * @return uint32_t 发送成功的数据长度
 */
uint32_t UdpServer::sendclientdata(char* buf, uint32_t len)
{
    auto strong_session = _last_session.lock();
    if (!strong_session) {
        PrintE("last session is null.");
        return 0;
    }

    return strong_session->senddata(buf,len);
}

/**
 * @brief 获取会话接收信息,避免session释放查询不到,包数量和总大小累加,序列号取最大值,速率采用当前值
 *
 * @param rcv_num   [out]接收包的数量(每次调用后session保存的值清0,因此这里累加)
 * @param rcv_seq   [out]接收包的最大序列号
 * @param rcv_len   [out]接收的字节总大小(每次调用后session保存的值清0,因此这里累加)
 * @param rcv_speed [out]接收速率
 */
void UdpServer::GetRcvInfo(uint64_t& rcv_num,uint64_t& rcv_seq,uint64_t& rcv_len,uint64_t& rcv_speed)
{
    uint64_t curr_seq = 0;
    rcv_speed = _socket->getRecvSpeed();// 服务端接收速率

    auto iter = _session_map->begin();
    while(iter != _session_map->end())
    {
        rcv_num += iter->second->GetPktNum();
        curr_seq += iter->second->GetSeq();
        rcv_len += iter->second->GetRcvLen();
        rcv_speed += iter->second->getSock()->getRecvSpeed();
        iter ++;
    }

    if(curr_seq > rcv_seq)
    {
        rcv_seq = curr_seq;
    }
}

} // namespace chw
