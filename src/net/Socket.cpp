// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include <type_traits>
#include "SocketBase.h"
#include "Socket.h"
#include "util.h"
#include "Logger.h"
#include "uv_errno.h"
#include "Semaphore.h"
#include "EventLoop.h"
//#include "Thread/WorkThreadPool.h"
using namespace std;

#define LOCK_GUARD(mtx) lock_guard<decltype(mtx)> lck(mtx)

namespace chw {

//StatisticImp(Socket)

static SockException toSockException(int error) {
    switch (error) {
        case 0:
        case UV_EAGAIN: return SockException(Err_success, "success");
        case UV_ECONNREFUSED: return SockException(Err_refused, uv_strerror(error), error);
        case UV_ETIMEDOUT: return SockException(Err_timeout, uv_strerror(error), error);
        case UV_ECONNRESET: return SockException(Err_reset, uv_strerror(error), error);
        default: return SockException(Err_other, uv_strerror(error), error);
    }
}

static SockException getSockErr(int sock, bool try_errno = true) {
    int error = 0, len = sizeof(int);
    getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&error, (socklen_t *)&len);
    if (error == 0) {
        if (try_errno) {
            error = get_uv_error(true);
        }
    } else {
        error = uv_translate_posix_error(error);
    }
    return toSockException(error);
}

Socket::Ptr Socket::createSocket(const EventLoop::Ptr &poller_in, bool enable_mutex) {
    //auto poller = poller_in ? poller_in : EventPollerPool::Instance().getPoller();
    std::weak_ptr<EventLoop> weak_poller = poller_in;
    return Socket::Ptr(new Socket(poller_in, enable_mutex), [weak_poller](Socket *ptr) {
        if (auto poller = weak_poller.lock()) {
            poller->async([ptr]() { delete ptr; });
        } else {
            delete ptr;
        }
    });
}

Socket::Socket(EventLoop::Ptr poller, bool enable_mutex)
    : _poller(std::move(poller))
    , _mtx_sock_fd(enable_mutex)
    , _mtx_event(enable_mutex)
    // , _mtx_send_buf_waiting(enable_mutex)
    // , _mtx_send_buf_sending(enable_mutex)
{
    setOnRead(nullptr);
    setOnErr(nullptr);
    setOnAccept(nullptr);
    // setOnFlush(nullptr);
    setOnBeforeAccept(nullptr);
    // setOnSendResult(nullptr);
}

Socket::~Socket() {
    closeSock();
}

void Socket::setOnRead(onReadCB cb) {
    if(cb) {
        _on_read = std::move(cb);
    } else {
        _on_read = [](Buffer::Ptr &buf, struct sockaddr *addr, int addr_len) {
            WarnL << "Socket not set read callback, data ignored: " << buf->RcvLen();
            buf->SetRcvLen(0);
        };
    }
    // onMultiReadCB cb2;
    // if (cb) {
    //     cb2 = [cb](Buffer::Ptr *buf, struct sockaddr_storage *addr, size_t count) {
    //         for (auto i = 0u; i < count; ++i) {
    //             cb(buf[i], (struct sockaddr *)(addr + i), sizeof(struct sockaddr_storage));
    //         }
    //     };
    // }
    // setOnMultiRead(std::move(cb2));
}

// void Socket::setOnMultiRead(onMultiReadCB cb) {
    // LOCK_GUARD(_mtx_event);
    // if (cb) {
    //     _on_multi_read = std::move(cb);
    // } else {
    //     _on_multi_read = [](Buffer::Ptr *buf, struct sockaddr_storage *addr, size_t count) {
    //         for (auto i = 0u; i < count; ++i) {
    //             WarnL << "Socket not set read callback, data ignored: " << buf[i]->size();
    //         }
    //     };
    // }
// }

void Socket::setOnErr(onErrCB cb) {
    LOCK_GUARD(_mtx_event);
    if (cb) {
        _on_err = std::move(cb);
    } else {
        _on_err = [](const SockException &err) { WarnL << "Socket not set err callback, err: " << err; };
    }
}

void Socket::setOnAccept(onAcceptCB cb) {
    LOCK_GUARD(_mtx_event);
    if (cb) {
        _on_accept = std::move(cb);
    } else {
        _on_accept = [](Socket::Ptr &sock, shared_ptr<void> &complete) { WarnL << "Socket not set accept callback, peer fd: " << sock->rawFD(); };
    }
}

// void Socket::setOnFlush(onFlush cb) {
//     LOCK_GUARD(_mtx_event);
//     if (cb) {
//         _on_flush = std::move(cb);
//     } else {
//         _on_flush = []() { return true; };
//     }
// }

void Socket::setOnBeforeAccept(onCreateSocket cb) {
    LOCK_GUARD(_mtx_event);
    if (cb) {
        _on_before_accept = std::move(cb);
    } else {
        _on_before_accept = [](const EventLoop::Ptr &poller) { return nullptr; };
    }
}

// void Socket::setOnSendResult(onSendResult cb) {
//     LOCK_GUARD(_mtx_event);
//     _send_result = std::move(cb);
// }

void Socket::connect(const string &url, uint16_t port, const onErrCB &con_cb_in, float timeout_sec, const string &local_ip, uint16_t local_port) {
    weak_ptr<Socket> weak_self = shared_from_this();
    // 因为涉及到异步回调，所以在poller线程中执行确保线程安全
    _poller->async([=] {
        if (auto strong_self = weak_self.lock()) {
            strong_self->connect_l(url, port, con_cb_in, timeout_sec, local_ip, local_port);
        }
    });
}

void Socket::connect_l(const string &url, uint16_t port, const onErrCB &con_cb_in, float timeout_sec, const string &local_ip, uint16_t local_port) {
    // 重置当前socket
    closeSock();

    weak_ptr<Socket> weak_self = shared_from_this();
    // 连接结果回调
    auto con_cb = [con_cb_in, weak_self](const SockException &err) {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }
        strong_self->_async_con_cb = nullptr;
        strong_self->_con_timer = nullptr;
        if (err) {
            strong_self->setSock(nullptr);
        }
        con_cb_in(err);
    };

    // 连接异步回调
    auto async_con_cb = std::make_shared<function<void(const SockNum::Ptr &)>>([weak_self, con_cb](const SockNum::Ptr &sock) {
        auto strong_self = weak_self.lock();
        if (!sock || !strong_self) {
            con_cb(SockException(Err_dns, get_uv_errmsg(true)));
            return;
        }

        // 监听该socket是否可写，可写表明已经连接服务器成功
        int result = strong_self->_poller->addEvent(sock->rawFd(), EventLoop::Event_Write | EventLoop::Event_Error, [weak_self, sock, con_cb](int event) {
            if (auto strong_self = weak_self.lock()) {
                strong_self->onConnected(sock, con_cb);
            }
        });

        if (result == -1) {
            con_cb(SockException(Err_other, std::string("add event to poller failed when start connect:") + get_uv_errmsg()));
        } else {
            // 先创建SockFD对象，防止SockNum由于未执行delEvent无法析构
            strong_self->setSock(sock);
        }
    });

    // 连接超时定时器
    _con_timer = std::make_shared<Timer>(timeout_sec,[weak_self, con_cb]() {
        con_cb(SockException(Err_timeout, uv_strerror(UV_ETIMEDOUT)));
        return false;
    }, _poller);

    if (SockUtil::isIP(url.data())) {
        auto fd = SockUtil::connect(url.data(), port, true, local_ip.data(), local_port);
        (*async_con_cb)(fd == -1 ? nullptr : std::make_shared<SockNum>(fd, SockNum::Sock_TCP));
    } else {
        // auto poller = _poller;
        // weak_ptr<function<void(const SockNum::Ptr &)>> weak_task = async_con_cb;
        // WorkThreadPool::Instance().getExecutor()->async([url, port, local_ip, local_port, weak_task, poller]() {
        //     // 阻塞式dns解析放在后台线程执行  [AUTO-TRANSLATED:e54694ea]
        //     //Blocking DNS resolution is executed in the background thread
        //     int fd = SockUtil::connect(url.data(), port, true, local_ip.data(), local_port);
        //     auto sock = fd == -1 ? nullptr : std::make_shared<SockNum>(fd, SockNum::Sock_TCP);
        //     poller->async([sock, weak_task]() {
        //         if (auto strong_task = weak_task.lock()) {
        //             (*strong_task)(sock);
        //         }
        //     });
        // });
        // _async_con_cb = async_con_cb;
    }
}

void Socket::onConnected(const SockNum::Ptr &sock, const onErrCB &cb) {
    auto err = getSockErr(sock->rawFd(), false);
    if (err) {
        // 连接失败
        cb(err);
        return;
    }

    // 更新地址信息
    setSock(sock);
    // 先删除之前的可写事件监听
    _poller->delEvent(sock->rawFd(), [sock](bool) {});
    if (!attachEvent(sock)) {
        // 连接失败
        cb(SockException(Err_other, "add event to poller failed when connected"));
        return;
    }

    {
        LOCK_GUARD(_mtx_sock_fd);
        if (_sock_fd) {
            _sock_fd->setConnected();
        }
    }
    // 连接成功
    cb(err);
}

bool Socket::attachEvent(const SockNum::Ptr &sock) {
    weak_ptr<Socket> weak_self = shared_from_this();
    if (sock->type() == SockNum::Sock_TCP_Server) {
        // tcp服务器
        auto result = _poller->addEvent(sock->rawFd(), EventLoop::Event_Read | EventLoop::Event_Error, [weak_self, sock](int event) {
            if (auto strong_self = weak_self.lock()) {
                strong_self->onAccept(sock, event);
            }
        });
        return -1 != result;
    }

    // tcp客户端或udp
    //auto read_buffer = _poller->getSharedBuffer(sock->type() == SockNum::Sock_UDP);
    auto result = _poller->addEvent(sock->rawFd(), EventLoop::Event_Read | EventLoop::Event_Error | EventLoop::Event_Write, [weak_self, sock/*, read_buffer*/](int event) {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }

        if (event & EventLoop::Event_Read) {
            strong_self->onRead(sock/*, read_buffer*/);
        }
        if (event & EventLoop::Event_Write) {
            strong_self->onWriteAble(sock);
        }
        if (event & EventLoop::Event_Error) {
            if (sock->type() == SockNum::Sock_UDP) {
                // udp ignore error
            } else {
                strong_self->emitErr(getSockErr(sock->rawFd()));
            }
        }
    });

    return -1 != result;
}

//接收数据
ssize_t Socket::onRead(const SockNum::Ptr &sock/*, const SocketRecvBuffer::Ptr &buffer*/) noexcept {
    //todo
    ssize_t ret = 0, nread = 0, count = 0;

    while (_enable_recv) {
        nread = /*buffer->*/recvFromSocket(sock->rawFd(), count);
        if (nread == 0) {
            if (sock->type() == SockNum::Sock_TCP) {
                emitErr(SockException(Err_eof, "end of file"));
            } else {
                WarnL << "Recv eof on udp socket[" << sock->rawFd() << "]";
            }
            return ret;
        }

        if (nread == -1) {
            auto err = get_uv_error(true);
            if (err != UV_EAGAIN) {
                if (sock->type() == SockNum::Sock_TCP) {
                    emitErr(toSockException(err));
                } else {
                    WarnL << "Recv err on udp socket[" << sock->rawFd() << "]: " << uv_strerror(err);
#ifdef _WIN32//chw
                    // windwos每次udp发送失败都会触发一次错误接收,此时应关闭本地udp socket
                    emitErr(toSockException(err));
#endif//_WIN32
                }
            } else {
                //跳出循环
            }
            return ret;
        }

        ret += nread;
        if (_enable_speed) {
            // 更新接收速率  [AUTO-TRANSLATED:1e24774c]
            //Update receive rate
            _recv_speed += nread;
        }

        // auto &buf = buffer->getBuffer(0);
        auto &addr = /*buffer->*/getAddress(0);
        try {
            // 此处捕获异常，目的是防止数据未读尽，epoll边沿触发失效的问题  [AUTO-TRANSLATED:2f3f813b]
            //Catch exception here, the purpose is to prevent data from not being read completely, and the epoll edge trigger fails
            LOCK_GUARD(_mtx_event);
            // _on_multi_read(&buf, &addr, count);
            _on_read(_buffer,(struct sockaddr *)&addr,sizeof(struct sockaddr_storage));
        } catch (std::exception &ex) {
            ErrorL << "Exception occurred when emit on_read: " << ex.what();
        }
    }
    return 0;
}

bool Socket::emitErr(const SockException &err) noexcept {
    if (_err_emit) {
        return true;
    }
    _err_emit = true;
    weak_ptr<Socket> weak_self = shared_from_this();
    _poller->async([weak_self, err]() {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }
        LOCK_GUARD(strong_self->_mtx_event);
        try {
            strong_self->_on_err(err);
        } catch (std::exception &ex) {
            ErrorL << "Exception occurred when emit on_err: " << ex.what();
        }
        // 延后关闭socket，只移除其io事件，防止Session对象析构时获取fd相关信息失败
        strong_self->closeSock(false);
    });
    return true;
}

// ssize_t Socket::send(const char *buf, size_t size, struct sockaddr *addr, socklen_t addr_len, bool try_flush) {
//     if (size <= 0) {
//         size = strlen(buf);
//         if (!size) {
//             return 0;
//         }
//     }
//     auto ptr = BufferRaw::create();
//     ptr->assign(buf, size);
//     return send(std::move(ptr), addr, addr_len, try_flush);
// }

// ssize_t Socket::send(string buf, struct sockaddr *addr, socklen_t addr_len, bool try_flush) {
//     return send(std::make_shared<BufferString>(std::move(buf)), addr, addr_len, try_flush);
// }

// ssize_t Socket::send(Buffer::Ptr buf, struct sockaddr *addr, socklen_t addr_len, bool try_flush) {
//     if (!addr) {
//         if (!_udp_send_dst) {
//             return send_l(std::move(buf), false, try_flush);
//         }
//         // 本次发送未指定目标地址，但是目标定制已通过bindPeerAddr指定  [AUTO-TRANSLATED:afb6ce35]
//         //This send did not specify a target address, but the target is customized through bindPeerAddr
//         addr = (struct sockaddr *)_udp_send_dst.get();
//         addr_len = SockUtil::get_sock_len(addr);
//     }
//     return send_l(std::make_shared<BufferSock>(std::move(buf), addr, addr_len), true, try_flush);
// }

// ssize_t Socket::send_l(Buffer::Ptr buf, bool is_buf_sock, bool try_flush) {
//     auto size = buf ? buf->size() : 0;
//     if (!size) {
//         return 0;
//     }

//     {
//         LOCK_GUARD(_mtx_send_buf_waiting);
//         _send_buf_waiting.emplace_back(std::move(buf), is_buf_sock);
//     }

//     if (try_flush) {
//         if (flushAll()) {
//             return -1;
//         }
//     }

//     return size;
// }

// int Socket::flushAll() {
//     LOCK_GUARD(_mtx_sock_fd);

//     if (!_sock_fd) {
//         // 如果已断开连接或者发送超时  [AUTO-TRANSLATED:2e25a648]
//         //If the connection is already disconnected or the send has timed out
//         return -1;
//     }
//     if (_sendable) {
//         // 该socket可写  [AUTO-TRANSLATED:9b37b658]
//         //The socket is writable
//         return flushData(_sock_fd->sockNum(), false) ? 0 : -1;
//     }

//     // 该socket不可写,判断发送超时  [AUTO-TRANSLATED:cad042e3]
//     //The socket is not writable, judging send timeout
//     if (_send_flush_ticker.elapsedTime() > _max_send_buffer_ms) {
//         // 如果发送列队中最老的数据距今超过超时时间限制，那么就断开socket连接  [AUTO-TRANSLATED:19ee680e]
//         //If the oldest data in the send queue exceeds the timeout limit, disconnect the socket connection
//         emitErr(SockException(Err_other, "socket send timeout"));
//         return -1;
//     }
//     return 0;
// }

// void Socket::onFlushed() {
//     bool flag;
//     {
//         LOCK_GUARD(_mtx_event);
//         flag = _on_flush();
//     }
//     if (!flag) {
//         setOnFlush(nullptr);
//     }
// }

void Socket::closeSock(bool close_fd) {
    _sendable = true;
    _enable_recv = true;
    _enable_speed = false;
    _con_timer = nullptr;
    _async_con_cb = nullptr;
    _send_flush_ticker.resetTime();

    {
        // LOCK_GUARD(_mtx_send_buf_waiting);
        // _send_buf_waiting.clear();
    }

    {
        // LOCK_GUARD(_mtx_send_buf_sending);
        // _send_buf_sending.clear();
    }

    {
        LOCK_GUARD(_mtx_sock_fd);
        if (close_fd) {
            _err_emit = false;
            _sock_fd = nullptr;
        } else if (_sock_fd) {
            _sock_fd->delEvent();
        }
    }
}

// size_t Socket::getSendBufferCount() {
//     size_t ret = 0;
//     {
//         LOCK_GUARD(_mtx_send_buf_waiting);
//         ret += _send_buf_waiting.size();
//     }

//     {
//         LOCK_GUARD(_mtx_send_buf_sending);
//         _send_buf_sending.for_each([&](BufferList::Ptr &buf) { ret += buf->count(); });
//     }
//     return ret;
// }

uint64_t Socket::elapsedTimeAfterFlushed() {
    return _send_flush_ticker.elapsedTime();
}

uint64_t Socket::getRecvSpeed() {
    _enable_speed = true;
    return _recv_speed.getSpeed();
}

uint64_t Socket::getSendSpeed() {
    _enable_speed = true;
    return _send_speed.getSpeed();
}

bool Socket::listen(uint16_t port, const string &local_ip, int backlog) {
    closeSock();
    int fd = SockUtil::listen(port, local_ip.data(), backlog);
    if (fd == -1) {
        return false;
    }
    return fromSock_l(std::make_shared<SockNum>(fd, SockNum::Sock_TCP_Server));
}

bool Socket::bindUdpSock(uint16_t port, const string &local_ip, bool enable_reuse) {
    closeSock();
    int fd = SockUtil::bindUdpSock(port, local_ip.data(), enable_reuse);
    if (fd == -1) {
        return false;
    }
    return fromSock_l(std::make_shared<SockNum>(fd, SockNum::Sock_UDP));
}

bool Socket::fromSock(int fd, SockNum::SockType type) {
    closeSock();
    SockUtil::setNoSigpipe(fd);
    SockUtil::setNoBlocked(fd);
    SockUtil::setCloExec(fd);
    return fromSock_l(std::make_shared<SockNum>(fd, type));
}

bool Socket::fromSock_l(SockNum::Ptr sock) {
    if (!attachEvent(sock)) {
        return false;
    }
    setSock(std::move(sock));
    return true;
}

// tcp服务端接入新连接
int Socket::onAccept(const SockNum::Ptr &sock, int event) noexcept {
    int fd;
    struct sockaddr_storage peer_addr;
    socklen_t addr_len = sizeof(peer_addr);
    while (true) {
        if (event & EventLoop::Event_Read) {
            do {
                fd = (int)accept(sock->rawFd(), (struct sockaddr *)&peer_addr, &addr_len);
            } while (-1 == fd && UV_EINTR == get_uv_error(true));

            if (fd == -1) {
                // accept失败
                int err = get_uv_error(true);
                if (err == UV_EAGAIN) {
                    // 没有新连接
                    return 0;
                }
                auto ex = toSockException(err);
                ErrorL << "Accept socket failed: " << ex.what();
                // 可能打开的文件描述符太多了:UV_EMFILE/UV_ENFILE
#if (defined(HAS_EPOLL) && !defined(_WIN32)) || defined(HAS_KQUEUE) 
                // 边缘触发，还需要手动再触发accept事件
                // wepoll, Edge-triggered (`EPOLLET`) mode isn't supported.
                std::weak_ptr<Socket> weak_self = shared_from_this();
                _poller->doDelayTask(100, [weak_self, sock]() {
                    if (auto strong_self = weak_self.lock()) {
                        // 100ms后再处理accept事件，说不定已经有空闲的fd
                        strong_self->onAccept(sock, EventLoop::Event_Read);
                    }
                    return 0;
                });
                // 暂时不处理accept事件，等待100ms后手动触发onAccept(只有EAGAIN读空后才能通过epoll再次触发事件) 
                return -1;
#else
                // 水平触发；休眠10ms，防止无谓的accept失败
                this_thread::sleep_for(std::chrono::milliseconds(10));
                // 暂时不处理accept事件，由于是水平触发，下次还会再次自动进入onAccept函数
                return -1;
#endif
            }

            SockUtil::setNoSigpipe(fd);
            SockUtil::setNoBlocked(fd);
            SockUtil::setNoDelay(fd);
            SockUtil::setSendBuf(fd);
            SockUtil::setRecvBuf(fd);
            SockUtil::setCloseWait(fd);
            SockUtil::setCloExec(fd);

            Socket::Ptr peer_sock;
            try {
                // 此处捕获异常，目的是防止socket未accept尽，epoll边沿触发失效的问题
                LOCK_GUARD(_mtx_event);
                // 拦截Socket对象的构造
                peer_sock = _on_before_accept(_poller);
            } catch (std::exception &ex) {
                ErrorL << "Exception occurred when emit on_before_accept: " << ex.what();
                close(fd);
                continue;
            }

            if (!peer_sock) {
                // 此处是默认构造行为，也就是子Socket共用父Socket的poll线程并且关闭互斥锁                
                peer_sock = Socket::createSocket(_poller, false);
            }

            auto sock = std::make_shared<SockNum>(fd, SockNum::Sock_TCP);
            // 设置好fd,以备在onAccept事件中可以正常访问该fd
            peer_sock->setSock(sock);
            // 赋值peer ip，防止在执行setSock时，fd已经被reset断开
            memcpy(&peer_sock->_peer_addr, &peer_addr, addr_len);

            shared_ptr<void> completed(nullptr, [peer_sock, sock](void *) {
                try {
                    // 然后把该fd加入poll监听(确保先触发onAccept事件然后再触发onRead等事件)                    
                    if (!peer_sock->attachEvent(sock)) {
                        // 加入poll监听失败，触发onErr事件，通知该Socket无效
                        peer_sock->emitErr(SockException(Err_eof, "add event to poller failed when accept a socket"));
                    }
                } catch (std::exception &ex) {
                    ErrorL << "Exception occurred: " << ex.what();
                }
            });

            try {
                // 此处捕获异常，目的是防止socket未accept尽，epoll边沿触发失效的问题
                LOCK_GUARD(_mtx_event);
                // 先触发onAccept事件，此时应该监听该Socket的onRead等事件
                _on_accept(peer_sock, completed);
            } catch (std::exception &ex) {
                ErrorL << "Exception occurred when emit on_accept: " << ex.what();
                continue;
            }
        }

        if (event & EventLoop::Event_Error) {
            auto ex = getSockErr(sock->rawFd());
            emitErr(ex);
            ErrorL << "TCP listener occurred a err: " << ex.what();
            return -1;
        }
    }
}

// 设置 _sock_fd 、本端地址、远端地址
void Socket::setSock(SockNum::Ptr sock) {
    LOCK_GUARD(_mtx_sock_fd);
    if (sock) {
        _sock_fd = std::make_shared<SockFD>(std::move(sock), _poller);
        SockUtil::get_sock_local_addr(_sock_fd->rawFd(), _local_addr);
        SockUtil::get_sock_peer_addr(_sock_fd->rawFd(), _peer_addr);
    } else {
        _sock_fd = nullptr;
    }
}

string Socket::get_local_ip() {
    LOCK_GUARD(_mtx_sock_fd);
    if (!_sock_fd) {
        return "";
    }
    return SockUtil::inet_ntoa((struct sockaddr *)&_local_addr);
}

uint16_t Socket::get_local_port() {
    LOCK_GUARD(_mtx_sock_fd);
    if (!_sock_fd) {
        return 0;
    }
    return SockUtil::inet_port((struct sockaddr *)&_local_addr);
}

string Socket::get_peer_ip() {
    LOCK_GUARD(_mtx_sock_fd);
    if (!_sock_fd) {
        return "";
    }
    if (_udp_send_dst) {
        return SockUtil::inet_ntoa((struct sockaddr *)_udp_send_dst.get());
    }
    return SockUtil::inet_ntoa((struct sockaddr *)&_peer_addr);
}

uint16_t Socket::get_peer_port() {
    LOCK_GUARD(_mtx_sock_fd);
    if (!_sock_fd) {
        return 0;
    }
    if (_udp_send_dst) {
        return SockUtil::inet_port((struct sockaddr *)_udp_send_dst.get());
    }
    return SockUtil::inet_port((struct sockaddr *)&_peer_addr);
}

string Socket::getIdentifier() const {
    static string class_name = "Socket: ";
    return class_name /*+ to_string(reinterpret_cast<uint64_t>(this))*/;//chw
}

// bool Socket::flushData(const SockNum::Ptr &sock, bool poller_thread) {
//     decltype(_send_buf_sending) send_buf_sending_tmp;
//     {
//         // 转移出二级缓存  [AUTO-TRANSLATED:a54264d2]
//         //Transfer out of the secondary cache
//         LOCK_GUARD(_mtx_send_buf_sending);
//         if (!_send_buf_sending.empty()) {
//             send_buf_sending_tmp.swap(_send_buf_sending);
//         }
//     }

//     if (send_buf_sending_tmp.empty()) {
//         _send_flush_ticker.resetTime();
//         do {
//             {
//                 // 二级发送缓存为空，那么我们接着消费一级缓存中的数据  [AUTO-TRANSLATED:8ddb2962]
//                 //The secondary send cache is empty, so we continue to consume data from the primary cache
//                 LOCK_GUARD(_mtx_send_buf_waiting);
//                 if (!_send_buf_waiting.empty()) {
//                     // 把一级缓中数数据放置到二级缓存中并清空  [AUTO-TRANSLATED:4884aa58]
//                     //Put the data from the first-level cache into the second-level cache and clear it
//                     LOCK_GUARD(_mtx_event);
//                     auto send_result = _enable_speed ? [this](const Buffer::Ptr &buffer, bool send_success) {
//                         if (send_success) {
//                             //更新发送速率  [AUTO-TRANSLATED:e35a1eba]
//                             //Update the sending rate
//                             _send_speed += buffer->size();
//                         }
//                         LOCK_GUARD(_mtx_event);
//                         if (_send_result) {
//                             _send_result(buffer, send_success);
//                         }
//                     } : _send_result;
//                     send_buf_sending_tmp.emplace_back(BufferList::create(std::move(_send_buf_waiting), std::move(send_result), sock->type() == SockNum::Sock_UDP));
//                     break;
//                 }
//             }
//             // 如果一级缓存也为空,那么说明所有数据均写入socket了  [AUTO-TRANSLATED:6ae9ef8a]
//             //If the first-level cache is also empty, it means that all data has been written to the socket
//             if (poller_thread) {
//                 // poller线程触发该函数，那么该socket应该已经加入了可写事件的监听；  [AUTO-TRANSLATED:5a8e123d]
//                 //The poller thread triggers this function, so the socket should have been added to the writable event listening
//                 // 那么在数据列队清空的情况下，我们需要关闭监听以免触发无意义的事件回调  [AUTO-TRANSLATED:0fb35573]
//                 //So, in the case of data queue clearing, we need to close the listening to avoid triggering meaningless event callbacks
//                 stopWriteAbleEvent(sock);
//                 onFlushed();
//             }
//             return true;
//         } while (false);
//     }

//     while (!send_buf_sending_tmp.empty()) {
//         auto &packet = send_buf_sending_tmp.front();
//         auto n = packet->send(sock->rawFd(), _sock_flags);
//         if (n > 0) {
//             // 全部或部分发送成功  [AUTO-TRANSLATED:0721ed7c]
//             //All or part of the data was sent successfully
//             if (packet->empty()) {
//                 // 全部发送成功  [AUTO-TRANSLATED:38a7d0ac]
//                 //All data was sent successfully
//                 send_buf_sending_tmp.pop_front();
//                 continue;
//             }
//             // 部分发送成功  [AUTO-TRANSLATED:bd6609dd]
//             //Part of the data was sent successfully
//             if (!poller_thread) {
//                 // 如果该函数是poller线程触发的，那么该socket应该已经加入了可写事件的监听，所以我们不需要再次加入监听  [AUTO-TRANSLATED:917049f0]
//                 //If this function is triggered by the poller thread, the socket should have been added to the writable event listening, so we don't need to add listening again
//                 startWriteAbleEvent(sock);
//             }
//             break;
//         }

//         // 一个都没发送成功  [AUTO-TRANSLATED:a3b4f257]
//         //None of the data was sent successfully
//         int err = get_uv_error(true);
//         if (err == UV_EAGAIN) {
//             // 等待下一次发送  [AUTO-TRANSLATED:22980496]
//             //Wait for the next send
//             if (!poller_thread) {
//                 // 如果该函数是poller线程触发的，那么该socket应该已经加入了可写事件的监听，所以我们不需要再次加入监听  [AUTO-TRANSLATED:917049f0]
//                 //If this function is triggered by the poller thread, the socket should have already been added to the writable event listener, so we don't need to add it again
//                 startWriteAbleEvent(sock);
//             }
//             break;
//         }

//         // 其他错误代码，发生异常  [AUTO-TRANSLATED:14cca084]
//         //Other error codes, an exception occurred
//         if (sock->type() == SockNum::Sock_UDP) {
//             // udp发送异常，把数据丢弃  [AUTO-TRANSLATED:3a7d095d]
//             //UDP send exception, discard the data
//             send_buf_sending_tmp.pop_front();
//             WarnL << "Send udp socket[" << sock << "] failed, data ignored: " << uv_strerror(err);
//             continue;
//         }
//         // tcp发送失败时，触发异常  [AUTO-TRANSLATED:06f06449]
//         //TCP send failed, trigger an exception
//         emitErr(toSockException(err));
//         return false;
//     }

//     // 回滚未发送完毕的数据  [AUTO-TRANSLATED:9f67c1be]
//     //Roll back the unsent data
//     if (!send_buf_sending_tmp.empty()) {
//         // 有剩余数据  [AUTO-TRANSLATED:14a89b15]
//         //There is remaining data
//         LOCK_GUARD(_mtx_send_buf_sending);
//         send_buf_sending_tmp.swap(_send_buf_sending);
//         _send_buf_sending.append(send_buf_sending_tmp);
//         // 二级缓存未全部发送完毕，说明该socket不可写，直接返回  [AUTO-TRANSLATED:2d7f9f2f]
//         //The secondary cache has not been sent completely, indicating that the socket is not writable, return directly
//         return true;
//     }

//     // 二级缓存已经全部发送完毕，说明该socket还可写，我们尝试继续写  [AUTO-TRANSLATED:2c2bc316]
//     //The secondary cache has been sent completely, indicating that the socket is still writable, we try to continue writing
//     // 如果是poller线程，我们尝试再次写一次(因为可能其他线程调用了send函数又有新数据了)  [AUTO-TRANSLATED:392684a8]
//     //If it's the poller thread, we try to write again (because other threads may have called the send function and there is new data)
//     return poller_thread ? flushData(sock, poller_thread) : true;
// }

void Socket::onWriteAble(const SockNum::Ptr &sock) {
    // bool empty_waiting;
    // bool empty_sending;
    // {
    //     LOCK_GUARD(_mtx_send_buf_waiting);
    //     empty_waiting = _send_buf_waiting.empty();
    // }

    // {
    //     LOCK_GUARD(_mtx_send_buf_sending);
    //     empty_sending = _send_buf_sending.empty();
    // }

    // if (empty_waiting && empty_sending) {
    //     // 数据已经清空了，我们停止监听可写事件
    //     stopWriteAbleEvent(sock);
    // } else {
    //     // socket可写，我们尝试发送剩余的数据
    //     flushData(sock, true);
    // }
}

void Socket::startWriteAbleEvent(const SockNum::Ptr &sock) {
    // 开始监听socket可写事件  [AUTO-TRANSLATED:31ba90c5]
    //Start listening for socket writable events
    _sendable = false;
    int flag = _enable_recv ? EventLoop::Event_Read : 0;
    _poller->modifyEvent(sock->rawFd(), flag | EventLoop::Event_Error | EventLoop::Event_Write, [sock](bool) {});
}

void Socket::stopWriteAbleEvent(const SockNum::Ptr &sock) {
    // 停止监听socket可写事件  [AUTO-TRANSLATED:4eb5b241]
    //Stop listening for socket writable events
    _sendable = true;
    int flag = _enable_recv ? EventLoop::Event_Read : 0;
    _poller->modifyEvent(sock->rawFd(), flag | EventLoop::Event_Error, [sock](bool) {});
}

void Socket::enableRecv(bool enabled) {
    if (_enable_recv == enabled) {
        return;
    }
    _enable_recv = enabled;
    int read_flag = _enable_recv ? EventLoop::Event_Read : 0;
    // 可写时，不监听可写事件  [AUTO-TRANSLATED:6a50e751]
    //Do not listen for writable events when writable
    int send_flag = _sendable ? 0 : EventLoop::Event_Write;
    _poller->modifyEvent(rawFD(), read_flag | send_flag | EventLoop::Event_Error);
}

int Socket::rawFD() const {
    LOCK_GUARD(_mtx_sock_fd);
    if (!_sock_fd) {
        return -1;
    }
    return _sock_fd->rawFd();
}

bool Socket::alive() const {
    LOCK_GUARD(_mtx_sock_fd);
    return _sock_fd && !_err_emit;
}

SockNum::SockType Socket::sockType() const {
    LOCK_GUARD(_mtx_sock_fd);
    if (!_sock_fd) {
        return SockNum::Sock_Invalid;
    }
    return _sock_fd->type();
}

void Socket::setSendTimeOutSecond(uint32_t second) {
    _max_send_buffer_ms = second * 1000;
}

bool Socket::isSocketBusy() const {
    return !_sendable.load();
}

const EventLoop::Ptr &Socket::getPoller() const {
    return _poller;
}

bool Socket::cloneSocket(const Socket &other) {
    closeSock();
    SockNum::Ptr sock;
    {
        LOCK_GUARD(other._mtx_sock_fd);
        if (!other._sock_fd) {
            WarnL << "sockfd of src socket is null";
            return false;
        }
        sock = other._sock_fd->sockNum();
    }
    return fromSock_l(sock);
}

bool Socket::bindPeerAddr(const struct sockaddr *dst_addr, socklen_t addr_len, bool soft_bind) {
    LOCK_GUARD(_mtx_sock_fd);
    if (!_sock_fd) {
        return false;
    }
    if (_sock_fd->type() != SockNum::Sock_UDP) {
        return false;
    }
    addr_len = addr_len ? addr_len : SockUtil::get_sock_len(dst_addr);
    if (soft_bind) {
        // 软绑定，只保存地址
        _udp_send_dst = std::make_shared<struct sockaddr_storage>();
        memcpy(_udp_send_dst.get(), dst_addr, addr_len);
    } else {
        // 硬绑定后，取消软绑定，防止memcpy目标地址的性能损失
        _udp_send_dst = nullptr;
        if (-1 == ::connect(_sock_fd->rawFd(), dst_addr, addr_len)) {
            WarnL << "Connect socket to peer address failed: " << SockUtil::inet_ntoa(dst_addr);
            return false;
        }
        memcpy(&_peer_addr, dst_addr, addr_len);
    }
    return true;
}

void Socket::setSendFlags(int flags) {
    _sock_flags = flags;
}

std::ostream &operator<<(std::ostream &ost, const SockException &err) {
    ost << err.getErrCode() << "(" << err.what() << ")";
    return ost;
}

void Socket::shutdown(const SockException &ex) {
    emitErr(ex);
}

void Socket::safeShutdown(const SockException &ex) {
    std::weak_ptr<Socket> weak_self = shared_from_this();
    _poller->async_first([weak_self, ex]() {
        if (auto strong_self = weak_self.lock()) {
            strong_self->shutdown(ex);
        }
    });
}

uint32_t Socket::send_i(char* buff, uint32_t len)
{
    LOCK_GUARD(_mtx_sock_fd);

    if (!_sock_fd) {
        PrintE("tcp not connected.");
        // 如果已断开连接或者发送超时
        return 0;
    }
    //发送失败由上层处理
    if (_sendable) {
        // 该socket可写
        uint32_t _snd_bytes = 0;
        if(len > 0 && buff != nullptr) {
            if(_sock_fd->type() == SockNum::Sock_TCP) {
                _snd_bytes =  SockUtil::send_tcp_data(_sock_fd->rawFd(),buff,len);
            } else {
                if(_udp_send_dst) {
                    struct sockaddr *addr = (struct sockaddr *)_udp_send_dst.get();
                    socklen_t addr_len = SockUtil::get_sock_len(addr);
                    _snd_bytes =  SockUtil::send_udp_data(_sock_fd->rawFd(),buff,len,addr,addr_len);
                } else {
                    struct sockaddr *addr = (struct sockaddr *)&_peer_addr;
                    socklen_t addr_len = SockUtil::get_sock_len(addr);
                    _snd_bytes =  SockUtil::send_udp_data(_sock_fd->rawFd(),buff,len,addr,addr_len);
                    // PrintD("_snd_bytes=%d", _snd_bytes);
                }
            }

            _send_speed += _snd_bytes;
            return _snd_bytes;
        }
    } else {
        PrintE("send able is false.");
        return 0;
    }

    // // 该socket不可写,判断发送超时
    // if (_send_flush_ticker.elapsedTime() > _max_send_buffer_ms) {
    //     // 如果发送列队中最老的数据距今超过超时时间限制，那么就断开socket连接
    //     emitErr(SockException(Err_other, "socket send timeout"));
    //     return chw::fail;
    // }

    return 0;
}

void Socket::enable_speed(bool enable)
{
    _enable_speed = enable;
}

} // namespace chw
