#include "TcpServer.h"
#include "uv_errno.h"
#include "onceToken.h"

using namespace std;

namespace chw {

// INSTANCE_IMP(SessionMap)
// StatisticImp(TcpServer)

TcpServer::TcpServer(const EventLoop::Ptr &poller) : Server(poller){
    // _poller = poller;
    // _multi_poller = !poller;
    setOnCreateSocket(nullptr);
}

void TcpServer::setupEvent() {
    _socket = createSocket(_poller);
    weak_ptr<TcpServer> weak_self = std::static_pointer_cast<TcpServer>(shared_from_this());
    _socket->setOnBeforeAccept([weak_self](const EventLoop::Ptr &poller) -> Socket::Ptr {
        if (auto strong_self = weak_self.lock()) {
            return strong_self->onBeforeAcceptConnection(poller);
        }
        return nullptr;
    });
    _socket->setOnAccept([weak_self](Socket::Ptr &sock, shared_ptr<void> &complete) {
        if (auto strong_self = weak_self.lock()) {
            strong_self->onAcceptConnection(sock);
            // auto ptr = sock->getPoller().get();
            // auto server = strong_self->getServer(ptr);
            // ptr->async([server, sock, complete]() {
            //     //该tcp客户端派发给对应线程的TcpServer服务器  [AUTO-TRANSLATED:662b882f]
            //     //This TCP client is dispatched to the corresponding thread of the TcpServer server
            //     server->onAcceptConnection(sock);
            // });
        }
    });
}

TcpServer::~TcpServer() {
    // if (_main_server && _socket && _socket->rawFD() != -1) {
    //     InfoL << "Close tcp server [" << _socket->get_local_ip() << "]: " << _socket->get_local_port();
    // }
    _timer.reset();
    //先关闭socket监听，防止收到新的连接
    _socket.reset();
    _session_map.clear();
    // _cloned_server.clear();
}

// uint16_t TcpServer::getPort() {
//     if (!_socket) {
//         return 0;
//     }
//     return _socket->get_local_port();
// }

// void TcpServer::setOnCreateSocket(Socket::onCreateSocket cb) {
//     if (cb) {
//         _on_create_socket = std::move(cb);
//     } else {
//         _on_create_socket = [](const EventLoop::Ptr &poller) {
//             return Socket::createSocket(poller, false);
//         };
//     }
//     // for (auto &pr : _cloned_server) {
//     //     pr.second->setOnCreateSocket(cb);
//     // }
// }

// TcpServer::Ptr TcpServer::onCreatServer(const EventLoop::Ptr &poller) {
//     return Ptr(new TcpServer(poller), [poller](TcpServer *ptr) { poller->async([ptr]() { delete ptr; }); });
// }

Socket::Ptr TcpServer::onBeforeAcceptConnection(const EventLoop::Ptr &poller) {
    assert(_poller->isCurrentThread());
    //此处改成自定义获取poller对象，防止负载不均衡
    // return createSocket(_multi_poller ? EventLoop::Instance().getPoller(false) : _poller);
    return createSocket(poller);
}

// void TcpServer::cloneFrom(const TcpServer &that) {
    // if (!that._socket) {
    //     throw std::invalid_argument("TcpServer::cloneFrom other with null socket");
    // }
    // setupEvent();
    // _main_server = false;
    // _on_create_socket = that._on_create_socket;
    // _session_alloc = that._session_alloc;
    // weak_ptr<TcpServer> weak_self = std::static_pointer_cast<TcpServer>(shared_from_this());
    // _timer = std::make_shared<Timer>(2.0f, [weak_self]() -> bool {
    //     auto strong_self = weak_self.lock();
    //     if (!strong_self) {
    //         return false;
    //     }
    //     strong_self->onManagerSession();
    //     return true;
    // }, _poller);
    // this->mINI::operator=(that);
    // _parent = static_pointer_cast<TcpServer>(const_cast<TcpServer &>(that).shared_from_this());
// }

// 接收到客户端连接请求
uint32_t TcpServer::onAcceptConnection(const Socket::Ptr &sock) {
    assert(_poller->isCurrentThread());
    weak_ptr<TcpServer> weak_self = std::static_pointer_cast<TcpServer>(shared_from_this());
    //创建一个Session;这里实现创建不同的服务会话实例
    auto session = _session_alloc(sock);
    // auto session = helper->session();
    // auto session = helper;
    //把本服务器的配置传递给Session
    // session->attachServer(*this);

    //_session_map::emplace肯定能成功
    auto success = _session_map.emplace(session.get(), session).second;
    assert(success == true);
    _last_session = session;
    PrintD("tcp client accept success, remote ip=%s, port=%u",sock->get_peer_ip().c_str(),sock->get_peer_port());

    weak_ptr<Session> weak_session = session;
    //会话接收数据事件
    sock->setOnRead([weak_session,weak_self](const Buffer::Ptr &buf, struct sockaddr *, int) {
        //获取会话强应用
        auto strong_session = weak_session.lock();
        if (!strong_session) {
            return;
        }

        //chw
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }
        strong_self->_last_session = strong_session;

        try {
            strong_session->onRecv(buf);
        } catch (SockException &ex) {
            strong_session->getSock()->shutdown(ex);
        } catch (exception &ex) {
            strong_session->getSock()->shutdown(SockException(Err_shutdown, ex.what()));
        }
    });

    Session *ptr = session.get();
    auto cls = ptr->className();
    //会话接收到错误事件
    sock->setOnErr([weak_self, weak_session, ptr, cls](const SockException &err) {
        //在本函数作用域结束时移除会话对象
        //目的是确保移除会话前执行其onError函数
        //同时避免其onError函数抛异常时没有移除会话对象
        onceToken token(nullptr, [&]() {
            //移除掉会话
            auto strong_self = weak_self.lock();
            if (!strong_self) {
                return;
            }

            assert(strong_self->_poller->isCurrentThread());
            if (!strong_self->_is_on_manager) {
                //该事件不是onManager时触发的，直接操作map
                strong_self->_session_map.erase(ptr);
            } else {
                //遍历map时不能直接删除元素
                strong_self->_poller->async([weak_self, ptr]() {
                    auto strong_self = weak_self.lock();
                    if (strong_self) {
                        strong_self->_session_map.erase(ptr);
                    }
                }, false);
            }
        });

        //获取会话强应用
        auto strong_session = weak_session.lock();
        if (strong_session) {
            //触发onError事件回调
            TraceP(strong_session->getSock()) << cls << " on err: " << err;
            strong_session->onError(err);
        }
    });
    return chw::success;
}

void TcpServer::start_l(uint16_t port, const std::string &host) {
    uint32_t backlog = 1024;
    setupEvent();

    //新建一个定时器定时管理这些tcp会话
    weak_ptr<TcpServer> weak_self = std::static_pointer_cast<TcpServer>(shared_from_this());
    _timer = std::make_shared<Timer>(2.0f, [weak_self]() -> bool {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return false;
        }
        strong_self->onManagerSession();
        return true;
    }, _poller);

    // if (_multi_poller) {
    //     EventPollerPool::Instance().for_each([&](const TaskExecutor::Ptr &executor) {
    //         EventLoop::Ptr poller = static_pointer_cast<EventLoop>(executor);
    //         if (poller == _poller) {
    //             return;
    //         }
    //         auto &serverRef = _cloned_server[poller.get()];
    //         if (!serverRef) {
    //             serverRef = onCreatServer(poller);
    //         }
    //         if (serverRef) {
    //             serverRef->cloneFrom(*this);
    //         }
    //     });
    // }

    if (!_socket->listen(port, host.c_str(), backlog)) {
        // 创建tcp监听失败，可能是由于端口占用或权限问题
        string err = (StrPrinter << "Listen on " << host << " " << port << " failed: " << get_uv_errmsg(true));
        throw std::runtime_error(err);
    }
    // for (auto &pr: _cloned_server) {
    //     // 启动子Server
    //     pr.second->_socket->cloneSocket(*_socket);
    // }

    InfoL << "TCP server listening on [" << host << "]: " << port;
}

void TcpServer::onManagerSession() {
    assert(_poller->isCurrentThread());

    onceToken token([&]() {
        _is_on_manager = true;
    }, [&]() {
        _is_on_manager = false;
    });

    for (auto &pr : _session_map) {
        //遍历时，可能触发onErr事件(也会操作_session_map)  [AUTO-TRANSLATED:7760b80d]
        //When traversing, the onErr event may be triggered (also operates on _session_map)
        try {
            pr.second->onManager();
        } catch (exception &ex) {
            WarnL << ex.what();
        }
    }
}

// Socket::Ptr TcpServer::createSocket(const EventLoop::Ptr &poller) {
//     return _on_create_socket(poller);
// }

// TcpServer::Ptr TcpServer::getServer(const EventLoop *poller) const {
//     auto parent = _parent.lock();
//     auto &ref = parent ? parent->_cloned_server : _cloned_server;
//     auto it = ref.find(poller);
//     if (it != ref.end()) {
//         //派发到cloned server  [AUTO-TRANSLATED:8765ab56]
//         //Dispatch to the cloned server
//         return it->second;
//     }
//     //派发到parent server  [AUTO-TRANSLATED:4cf34169]
//     //Dispatch to the parent server
//     return static_pointer_cast<TcpServer>(parent ? parent : const_cast<TcpServer *>(this)->shared_from_this());
// }

// Session::Ptr TcpServer::createSession(const Socket::Ptr &sock) {
//     return getServer(sock->getPoller().get())->onAcceptConnection(sock);
// }

uint32_t TcpServer::sendclientdata(uint8_t* buf, uint32_t len)
{
    auto strong_session = _last_session.lock();
    if (!strong_session) {
        PrintE("last session is null.");
        return 0;
    }

    return strong_session->senddata(buf,len);
}

} //namespace chw

