﻿// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __SOCKET_H
#define __SOCKET_H

#include <memory>
#include <string>
#include <mutex>
#include <atomic>
#include <sstream>
#include <functional>
#include <string>
#include "SpeedStatistic.h"
#include "SocketBase.h"
#include "Timer.h"
#include "EventLoop.h"
//#include "BufferSock.h"
#include "Buffer.h"
#include "uv_errno.h"

namespace chw {

#if defined(MSG_NOSIGNAL)
#define FLAG_NOSIGNAL MSG_NOSIGNAL
#else
#define FLAG_NOSIGNAL 0
#endif //MSG_NOSIGNAL

#if defined(MSG_MORE)
#define FLAG_MORE MSG_MORE
#else
#define FLAG_MORE 0
#endif //MSG_MORE

#if defined(MSG_DONTWAIT)
#define FLAG_DONTWAIT MSG_DONTWAIT
#else
#define FLAG_DONTWAIT 0
#endif //MSG_DONTWAIT

//默认的socket flags:不触发SIGPIPE,非阻塞发送
#define SOCKET_DEFAULE_FLAGS (FLAG_NOSIGNAL | FLAG_DONTWAIT )
    
//发送超时时间，如果在规定时间内一直没有发送数据成功，那么将触发onErr事件
#define SEND_TIME_OUT_SEC 10
    
//错误类型枚举
typedef enum {
    Err_success = 0, //成功 success
    Err_eof, //eof
    Err_timeout, //超时 socket timeout
    Err_refused,//连接被拒绝 socket refused
    Err_reset,//连接被重置  socket reset
    Err_dns,//dns解析失败 dns resolve failed
    Err_shutdown,//主动关闭 socket shutdown
    Err_other = 0xFF,//其他错误 other error
} ErrCode;

//错误信息类，重载bool()，_code为0则是无异常
class SockException : public std::exception {
public:
    SockException(ErrCode code = Err_success, const std::string &msg = "", int custom_code = 0) {
        _msg = msg;
        _code = code;
        _custom_code = custom_code;
    }

    //重置错误
    void reset(ErrCode code, const std::string &msg, int custom_code = 0) {
        _msg = msg;
        _code = code;
        _custom_code = custom_code;
    }

    //错误提
    const char *what() const noexcept override {
        return _msg.c_str();
    }

    //错误代码
    ErrCode getErrCode() const {
        return _code;
    }

    //用户自定义错误代码
    int getCustomCode() const {
        return _custom_code;
    }

    //判断是否真的有错
    operator bool() const {
        return _code != Err_success;
    }

private:
    ErrCode _code;
    int _custom_code = 0;
    std::string _msg;
};

//std::cout等输出流可以直接输出SockException对象
std::ostream &operator<<(std::ostream &ost, const SockException &err);

class SockNum {
public:
    using Ptr = std::shared_ptr<SockNum>;

    typedef enum {
        Sock_Invalid = -1,
        Sock_TCP = 0,
        Sock_UDP = 1,
        Sock_TCP_Server = 2
    } SockType;

    SockNum(int fd, SockType type) {
        _fd = fd;
        _type = type;
    }

    ~SockNum() {
#if defined (OS_IPHONE)
        unsetSocketOfIOS(_fd);
#endif //OS_IPHONE
        // 停止socket收发能力
        #if defined(_WIN32)
        ::shutdown(_fd, SD_BOTH);
        #else
        ::shutdown(_fd, SHUT_RDWR);
        #endif
        close(_fd);
    }

    int rawFd() const {
        return _fd;
    }

    SockType type() {
        return _type;
    }

    void setConnected() {
#if defined (OS_IPHONE)
        setSocketOfIOS(_fd);
#endif //OS_IPHONE
    }

#if defined (OS_IPHONE)
private:
    void *readStream=nullptr;
    void *writeStream=nullptr;
    bool setSocketOfIOS(int socket);
    void unsetSocketOfIOS(int socket);
#endif //OS_IPHONE

private:
    int _fd;
    SockType _type;
};

//socket 文件描述符的包装
//在析构时自动溢出监听并close套接字
//防止描述符溢出
class SockFD : public noncopyable {
public:
    using Ptr = std::shared_ptr<SockFD>;

    /**
     * 创建一个fd对象
     * @param num 文件描述符，int数字
     * @param poller 事件监听器
     */
    SockFD(SockNum::Ptr num, const EventLoop::Ptr &poller) {
        _num = std::move(num);
        _poller = poller;
    }

    /**
     * 复制一个fd对象
     * @param that 源对象
     * @param poller 事件监听器
     */
    SockFD(const SockFD &that, const EventLoop::Ptr &poller) {
        _num = that._num;
        _poller = poller;
        if (_poller == that._poller) {
            throw std::invalid_argument("Copy a SockFD with same poller");
        }
    }

     ~SockFD() { delEvent(); }

    void delEvent() {
        if (_poller) {
            auto num = _num;
            // 移除io事件成功后再close fd
            _poller->delEvent(num->rawFd(), [num](bool) {});
            _poller = nullptr;
        }
    }

    void setConnected() {
        _num->setConnected();
    }

    int rawFd() const {
        return _num->rawFd();
    }

    const SockNum::Ptr& sockNum() const {
        return _num;
    }

    SockNum::SockType type() {
        return _num->type();
    }

    const EventLoop::Ptr& getPoller() const {
        return _poller;
    }

private:
    SockNum::Ptr _num;
    EventLoop::Ptr _poller;
};

template<class Mtx = std::recursive_mutex>
class MutexWrapper {
public:
    MutexWrapper(bool enable) {
        _enable = enable;
    }

    ~MutexWrapper() = default;

    inline void lock() {
        if (_enable) {
            _mtx.lock();
        }
    }

    inline void unlock() {
        if (_enable) {
            _mtx.unlock();
        }
    }

private:
    bool _enable;
    Mtx _mtx;
};

#define TraceP(ptr) TraceL << ptr->getIdentifier() << "(" << ptr->get_peer_ip() << ":" << ptr->get_peer_port() << ") "
#define DebugP(ptr) DebugL << ptr->getIdentifier() << "(" << ptr->get_peer_ip() << ":" << ptr->get_peer_port() << ") "
#define InfoP(ptr) InfoL << ptr->getIdentifier() << "(" << ptr->get_peer_ip() << ":" << ptr->get_peer_port() << ") "
#define WarnP(ptr) WarnL << ptr->getIdentifier() << "(" << ptr->get_peer_ip() << ":" << ptr->get_peer_port() << ") "
#define ErrorP(ptr) ErrorL << ptr->getIdentifier() << "(" << ptr->get_peer_ip() << ":" << ptr->get_peer_port() << ") "

//异步IO Socket对象，包括tcp客户端、服务器和udp套接字
class Socket : public std::enable_shared_from_this<Socket>, public noncopyable {
public:
    using Ptr = std::shared_ptr<Socket>;
    //接收数据回调
    using onReadCB = std::function<void(Buffer::Ptr &buf, struct sockaddr *addr, int addr_len)>;
    // using onMultiReadCB = std::function<void(Buffer::Ptr *buf, struct sockaddr_storage *addr, size_t count)>;

    //发生错误回调
    using onErrCB = std::function<void(const SockException &err)>;
    //tcp监听接收到连接请求
    using onAcceptCB = std::function<void(Socket::Ptr &sock, std::shared_ptr<void> &complete)>;
    //socket发送缓存清空事件，返回true代表下次继续监听该事件，否则停止
    using onFlush = std::function<bool()>;
    //在接收到连接请求前，拦截Socket默认生成方式
    using onCreateSocket = std::function<Ptr(const EventLoop::Ptr &poller)>;
    //发送buffer成功与否回调
    // using onSendResult = BufferList::SendResult;

    /**
     * 构造socket对象，尚未有实质操作
     * @param poller 绑定的poller线程
     * @param enable_mutex 是否启用互斥锁(接口是否线程安全)
    */
    static Ptr createSocket(const EventLoop::Ptr &poller, bool enable_mutex = true);
    ~Socket();

    /**
     * 创建tcp客户端并异步连接服务器
     * @param url 目标服务器ip或域名
     * @param port 目标服务器端口
     * @param con_cb 结果回调
     * @param timeout_sec 超时时间
     * @param local_ip 绑定本地网卡ip
     * @param local_port 绑定本地网卡端口号
     */
    void connect(const std::string &url, uint16_t port, const onErrCB &con_cb, float timeout_sec = 5, const std::string &local_ip = "::", uint16_t local_port = 0);

    /**
     * 创建tcp监听服务器
     * @param port 监听端口，0则随机
     * @param local_ip 监听的网卡ip
     * @param backlog tcp最大积压数
     * @return 是否成功
     */
    bool listen(uint16_t port, const std::string &local_ip = "::", int backlog = 1024);

    /**
     * 创建udp套接字,udp是无连接的，所以可以作为服务器和客户端
     * @param port 绑定的端口为0则随机
     * @param local_ip 绑定的网卡ip
     * @return 是否成功
     */
    bool bindUdpSock(uint16_t port, const std::string &local_ip = "::", bool enable_reuse = true);

    /**
     * 包装外部fd，本对象负责close fd
     * 内部会设置fd为NoBlocked,NoSigpipe,CloExec
     * 其他设置需要自行使用SockUtil进行设置
     */
    bool fromSock(int fd, SockNum::SockType type);

    /**
     * 从另外一个Socket克隆
     * 目的是一个socket可以被多个poller对象监听，提高性能或实现Socket归属线程的迁移
     * @param other 原始的socket对象
     * @return 是否成功
     */
    bool cloneSocket(const Socket &other);

    ////////////设置事件回调////////////

    /**
     * 设置数据接收回调,tcp或udp客户端有效
     * @param cb 回调对象
     */
    void setOnRead(onReadCB cb);
    // void setOnMultiRead(onMultiReadCB cb);

    /**
     * 设置异常事件(包括eof等)回调
     * @param cb 回调对象
     */
    void setOnErr(onErrCB cb);

    /**
     * 设置tcp监听接收到连接回调
     * @param cb 回调对象
     */
    void setOnAccept(onAcceptCB cb);

    // /**
    //  * 设置socket写缓存清空事件回调
    //  * 通过该回调可以实现发送流控
    //  * @param cb 回调对象
    //  */
    // void setOnFlush(onFlush cb);

    /**
     * 设置accept时，socket构造事件回调
     * @param cb 回调
     */
    void setOnBeforeAccept(onCreateSocket cb);

    // /**
    //  * 设置发送buffer结果回调
    //  * @param cb 回调
    //  * Set send buffer result callback
    //  * @param cb callback
     
    //  * [AUTO-TRANSLATED:1edb77bb]
    //  */
    // void setOnSendResult(onSendResult cb);

    ////////////发送数据相关接口////////////  [AUTO-TRANSLATED:c14ca1a7]
    ////////////Data sending related interfaces////////////

    // /**
    //  * 发送数据指针
    //  * @param buf 数据指针
    //  * @param size 数据长度
    //  * @param addr 目标地址
    //  * @param addr_len 目标地址长度
    //  * @param try_flush 是否尝试写socket
    //  * @return -1代表失败(socket无效)，0代表数据长度为0，否则返回数据长度
    //  */
    // ssize_t send(const char *buf, size_t size = 0, struct sockaddr *addr = nullptr, socklen_t addr_len = 0, bool try_flush = true);

    // /**
    //  * 发送string
    //  * Send string
     
    //  * [AUTO-TRANSLATED:f9dfdfcf]
    //  */
    // ssize_t send(std::string buf, struct sockaddr *addr = nullptr, socklen_t addr_len = 0, bool try_flush = true);

    // /**
    //  * 发送Buffer对象，Socket对象发送数据的统一出口
    //  * socket对象发送数据的统一出口
    //  */
    // ssize_t send(Buffer::Ptr buf, struct sockaddr *addr = nullptr, socklen_t addr_len = 0, bool try_flush = true);

    // /**
    //  * 尝试将所有数据写socket
    //  * @return -1代表失败(socket无效或者发送超时)，0代表成功?
    //  */
    // int flushAll();

    /**
     * 关闭socket且触发onErr回调，onErr回调将在poller线程中进行
     * @param err 错误原因
     * @return 是否成功触发onErr回调
     */
    bool emitErr(const SockException &err) noexcept;

    /**
     * 关闭或开启数据接收
     * @param enabled 是否开启
     */
    void enableRecv(bool enabled);

    /**
     * 获取裸文件描述符，请勿进行close操作(因为Socket对象会管理其生命周期)
     * @return 文件描述符
     */
    int rawFD() const;

    /**
     * tcp客户端是否处于连接状态
     * 支持Sock_TCP类型socket
     */
    bool alive() const;

    /**
     * 返回socket类型
     */
    SockNum::SockType sockType() const;

    /**
     * 设置发送超时主动断开时间;默认10秒
     * @param second 发送超时数据，单位秒
     */
    void setSendTimeOutSecond(uint32_t second);

    /**
     * 套接字是否忙，如果套接字写缓存已满则返回true
     * @return 套接字是否忙
     */
    bool isSocketBusy() const;

    /**
     * 获取poller线程对象
     * @return poller线程对象
     */
    const EventLoop::Ptr &getPoller() const;

    /**
     * 绑定udp 目标地址，后续发送时就不用再单独指定了
     * @param dst_addr 目标地址
     * @param addr_len 目标地址长度
     * @param soft_bind 是否软绑定，软绑定时不调用udp connect接口，只保存目标地址信息，发送时再传递到sendto函数
     * @return 是否成功
     */
    bool bindPeerAddr(const struct sockaddr *dst_addr, socklen_t addr_len = 0, bool soft_bind = false);

    /**
     * 设置发送flags
     * @param flags 发送的flag
     */
    void setSendFlags(int flags = SOCKET_DEFAULE_FLAGS);

    /**
     * 关闭套接字
     * @param close_fd 是否关闭fd还是只移除io事件监听
     */
    void closeSock(bool close_fd = true);

    // /**
    //  * 获取发送缓存包个数(不是字节数)
    // size_t getSendBufferCount();

    /**
     * 获取上次socket发送缓存清空至今的毫秒数,单位毫秒
     */
    uint64_t elapsedTimeAfterFlushed();

    /**
     * 获取接收速率，单位bytes/s
     */
    uint64_t getRecvSpeed();

    /**
     * 获取发送速率，单位bytes/s
     */
    uint64_t getSendSpeed();

    //获取本机ip
    std::string get_local_ip();
    //获取本机端口号
    uint16_t get_local_port();
    //获取对方ip
    std::string get_peer_ip();
    //获取对方端口号
    uint16_t get_peer_port();
    //获取标识符
    std::string getIdentifier() const;

private:
    /**
     * @brief Socket私有构造函数，只能通过静态方法 createSocket 创建
     * 
     * @param poller        [in]绑定的事件循环线程
     * @param enable_mutex  [in]是否启用线程锁,false则_mtx_sock_fd、_mtx_event等锁失效
     */
    Socket(EventLoop::Ptr poller, bool enable_mutex = true);

    /**
     * @brief 设置 _sock_fd 、本端地址、远端地址
     * 
     * @param sock [in]fd
     */
    void setSock(SockNum::Ptr sock);

    /**
     * @brief tcp服务端epoll事件回调，Event_Read（新链接接入）和Event_Error（错误）
     * 
     * @param sock  [in]tcp服务端fd
     * @param event [in]epoll事件
     * @return int 失败返回-1，没有新连接了返回0
     */
    int onAccept(const SockNum::Ptr &sock, int event) noexcept;

    /**
     * @brief epoll可读事件回调
     * @param sock [in]fd
     */
    ssize_t onRead(const SockNum::Ptr &sock/*, const SocketRecvBuffer::Ptr &buffer*/) noexcept;

    /**
     * @brief epoll可写事件回调
     * 
     * @param sock [in]fd
     */
    void onWriteAble(const SockNum::Ptr &sock);

    /**
     * @brief tcp连接结果回调
     * 
     * @param sock  [in]fd
     * @param cb    回调
     */
    void onConnected(const SockNum::Ptr &sock, const onErrCB &cb);
    // void onFlushed();

    /**
     * @brief 开始监听socket可写事件
     * 
     * @param sock [in]fd
     */
    void startWriteAbleEvent(const SockNum::Ptr &sock);

    /**
     * @brief 停止监听socket可写事件
     * 
     * @param sock [in]fd
     */
    void stopWriteAbleEvent(const SockNum::Ptr &sock);
    // bool flushData(const SockNum::Ptr &sock, bool poller_thread);

    /**
     * @brief 激活epoll事件，tcp服务端监听Event_Read | Event_Error,tcp/udp客户端监听Event_Read | Event_Error | Event_Write
     * 
     * @param sock      [in]监听fd
     * @return true     成功
     * @return false    失败
     */
    bool attachEvent(const SockNum::Ptr &sock);
    // ssize_t send_l(Buffer::Ptr buf, bool is_buf_sock, bool try_flush = true);

    /**
     * @brief tcp客户端异步连接到服务端
     * 
     * @param url           [in]服务端ip
     * @param port          [in]服务端port
     * @param con_cb_in     [in]连接结果回调
     * @param timeout_sec   [in]连接超时时长，单位秒
     * @param local_ip      [in]本地绑定的ip
     * @param local_port    [in]本地绑定的port
     */
    void connect_l(const std::string &url, uint16_t port, const onErrCB &con_cb_in, float timeout_sec, const std::string &local_ip, uint16_t local_port);
    
    /**
     * @brief 激活epoll事件，设置 _sock_fd
     * 
     * @param sock      fd
     * @return true     成功
     * @return false    失败
     */
    bool fromSock_l(SockNum::Ptr sock);

private:
    // send socket时的flag
    int _sock_flags = SOCKET_DEFAULE_FLAGS;
    // 最大发送缓存，单位毫秒，距上次发送缓存清空时间不能超过该参数
    uint32_t _max_send_buffer_ms = SEND_TIME_OUT_SEC * 1000;
    // 控制是否接收监听socket可读事件，关闭后可用于流量控制
    std::atomic<bool> _enable_recv { true };
    // 标记该socket是否可写，socket写缓存满了就不可写
    std::atomic<bool> _sendable { true };
    // 是否已经触发err回调了
    bool _err_emit = false;
    // 是否启用网速统计
    bool _enable_speed = false;
    // udp发送目标地址
    std::shared_ptr<struct sockaddr_storage> _udp_send_dst;

    // 接收速率统计
    BytesSpeed _recv_speed;
    // 发送速率统计
    BytesSpeed _send_speed;

    // tcp连接超时定时器
    Timer::Ptr _con_timer;
    // tcp连接结果回调对象
    std::shared_ptr<void> _async_con_cb;

    // 记录上次发送缓存(包括socket写缓存、应用层缓存)清空的计时器
    Ticker _send_flush_ticker;
    // socket fd的抽象类
    SockFD::Ptr _sock_fd;
    // 本socket绑定的poller线程，事件触发于此线程
    EventLoop::Ptr _poller;
    // 跨线程访问_sock_fd时需要上锁
    mutable MutexWrapper<std::recursive_mutex> _mtx_sock_fd;

    // socket异常事件(比如说断开)
    onErrCB _on_err;
    // 收到数据事件
    // onMultiReadCB _on_multi_read;
    // socket缓存清空事件(可用于发送流速控制)
    onFlush _on_flush;
    // tcp监听收到accept请求事件
    onAcceptCB _on_accept;
    // tcp监听收到accept请求，自定义创建peer Socket事件(可以控制子Socket绑定到其他poller线程)
    onCreateSocket _on_before_accept;
    // 设置上述回调函数的锁
    MutexWrapper<std::recursive_mutex> _mtx_event;

    // 一级发送缓存, socket可写时，会把一级缓存批量送入到二级缓存
    // List<std::pair<Buffer::Ptr, bool>> _send_buf_waiting;
    // 一级发送缓存锁
    // MutexWrapper<std::recursive_mutex> _mtx_send_buf_waiting;
    // 二级发送缓存, socket可写时，会把二级缓存批量写入到socket
    // List<BufferList::Ptr> _send_buf_sending;
    // 二级发送缓存锁
    // MutexWrapper<std::recursive_mutex> _mtx_send_buf_sending;
    // 发送buffer结果回调
    // BufferList::SendResult _send_result;
    // 对象个数统计
    //ObjectStatistic<Socket> _statistic;

    // 链接缓存地址,防止tcp reset 导致无法获取对端的地址
    struct sockaddr_storage _local_addr;
    struct sockaddr_storage _peer_addr;

//chw
public:
    /**
     * 触发onErr事件
     */
    void shutdown(const SockException &ex = SockException(Err_shutdown, "self shutdown"));

    /**
     * 线程安全的脱离 Server 并触发 onError 事件
     * @param ex 触发 onError 事件的原因
     */
    void safeShutdown(const SockException &ex = SockException(Err_shutdown, "self shutdown"));

    /**
     * @brief 不缓存，立刻发送数据；失败时丢弃数据，并建议上层断开重联
     * tcp:发送失败会阻塞尝试一定次数重发
     * udp:发送失败立即返回
     * 
     * @param buff 数据
     * @param len  数据长度
     * @return uint32_t 发送成功的数据长度
     */
    uint32_t send_i(char* buff, uint32_t len);
    //todo:创建缓存，当数据达到一定数量时再执行系统调用，减少syscall，如果一定时间内没有达到一定数量也执行系统调用。
    //注意: 发送失败或部分发送成功会丢弃包,此时建议业务模块触发tcp断开重联
    //todo:方案2：发送失败时监听可写事件，没有发送成功的先放入缓存，可写时再发送（一直发送失败会出现大量缓存积压，不利于业务快速反映）
    // _max_send_buffer_ms/_send_flush_ticker 控制一直发送失败的超时

    /**
     * @brief 先把数据拷贝到Buffer，Buffer足够大时执行系统调用send，适合小包较多的数据，仅用于tcp
     * epoll可写时执行发送，不可写时暂停发送，需要设置发送失败超时时长
     * 
     * @param buff  数据
     * @param len   数据长度
     * @return uint32_t 发送成功的数据长度
     */
    uint32_t send_b(char* buff, uint32_t len);

    /**
     * @brief 先把数据Buffer放入一个list，当epoll不可写时暂时挂起，不阻塞线程
     * epoll可写时执行发送，不可写时暂停发送，需要设置发送失败超时时长
     * todo:可设置buff是否托管，托管则无需拷贝直接放入list，不托管则拷贝到新Buffer后放入一个list，均在发送完成后释放
     * 
     * @param buff  数据
     * @param len   数据长度
     * @return uint32_t 发送成功的数据长度
     */
    uint32_t send_l(char* buff, uint32_t len, bool give_up_owner){return 0;};

    /**
     * @brief 是否启动测速
     * 
     * @param enable true启动测速，false关闭测速
     */
    void enable_speed(bool enable);
private:
    onReadCB _on_read;
    Buffer::Ptr _SndBuffer;//用于 send_b 的发送缓存区

private://SocketRecvFromBuffer
    Buffer::Ptr _buffer;
    struct sockaddr_storage _address;

    // 只接收,数据交给上层处理
    ssize_t recvFromSocket(int fd, ssize_t &count) {
        ssize_t nread;
        socklen_t len = sizeof(_address);
        if (!_buffer) {
            // allocBuffer();
            _buffer = std::make_shared<Buffer>();

            uint32_t ret = chw::success;
            if(_sock_fd->type() == SockNum::Sock_UDP) {
                ret = _buffer->SetCapacity(RAW_BUFFER_SIZE);
            } else {
                ret = _buffer->SetCapacity(TCP_BUFFER_SIZE);
            }

            if(ret == chw::fail) {
                shutdown();
            } else {
                _buffer->Reset0();
            }
        }

        do {
            // nread = recvfrom(fd, _buffer->data(), _buffer->getCapacity() - 1, 0, (struct sockaddr *)&_address, &len);
            nread = recvfrom(fd, (char*)_buffer->data() + _buffer->Size(), _buffer->Idle(), 0, (struct sockaddr *)&_address, &len);
        } while (-1 == nread && UV_EINTR == get_uv_error(true));

        if (nread > 0) {
            _buffer->SetSize(_buffer->Size() + nread);
            count = 1;
            // _buffer->data()[nread] = '\0';
            // std::static_pointer_cast<BufferRaw>(_buffer)->setSize(nread);
        }
        return nread;
    }

    struct sockaddr_storage &getAddress(size_t index) { return _address; }
};

}  // namespace chw
#endif //__SOCKET_H
