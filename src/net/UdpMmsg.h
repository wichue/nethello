// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __UDP_MMSG_H
#define __UDP_MMSG_H

#include <memory>
#include "util.h"
#include "Buffer.h"
#include "uv_errno.h"

#if defined(__linux__) || defined(__linux)
#include <sys/socket.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef MSG_WAITFORONE
#define MSG_WAITFORONE  0x10000
#endif

#ifndef HAVE_MMSG_HDR
struct mmsghdr {
        struct msghdr   msg_hdr;
        unsigned        msg_len;
};
#endif

#ifndef HAVE_SENDMMSG_API
#include <unistd.h>
#include <sys/syscall.h>
static inline int sendmmsg(int fd, struct mmsghdr *mmsg,
                unsigned vlen, unsigned flags)
{
        return syscall(__NR_sendmmsg, fd, mmsg, vlen, flags);
}
#endif

#ifndef HAVE_RECVMMSG_API
#include <unistd.h>
#include <sys/syscall.h>
static inline int recvmmsg(int fd, struct mmsghdr *mmsg,
                unsigned vlen, unsigned flags, struct timespec *timeout)
{
        return syscall(__NR_recvmmsg, fd, mmsg, vlen, flags, timeout);
}
#endif

#endif// defined(__linux__) || defined(__linux)

namespace chw {

class BufferSock  {
public:
    using Ptr = std::shared_ptr<BufferSock>;
    BufferSock(Buffer::Ptr ptr, struct sockaddr *addr = nullptr, int addr_len = 0);
    ~BufferSock()  = default;

    char *data() const ;
    size_t size() const ;
    const struct sockaddr *sockaddr() const;
    socklen_t  socklen() const;

private:
    int _addr_len = 0;
    struct sockaddr_storage _addr;
    Buffer::Ptr _buffer;
};

class BufferList : public noncopyable {
public:
    using Ptr = std::shared_ptr<BufferList>;
    using SendResult = std::function<void(const Buffer::Ptr &buffer, bool send_success)>;

    BufferList() = default;
    virtual ~BufferList() = default;

    virtual bool empty() = 0;
    virtual size_t count() = 0;
    virtual ssize_t send(int fd, int flags) = 0;

    static Ptr create(List<std::pair<Buffer::Ptr, bool> > list, SendResult cb, bool is_udp);

private:
    //对象个数统计  [AUTO-TRANSLATED:3b43e8c2]
    //Object count statistics
    // ObjectStatistic<BufferList> _statistic;
};


/////////////////////////////////////// BufferCallBack ///////////////////////////////////////

class BufferCallBack {
public:
    BufferCallBack(List<std::pair<Buffer::Ptr, bool> > list, BufferList::SendResult cb)
        : _cb(std::move(cb))
        , _pkt_list(std::move(list)) {}

    ~BufferCallBack() {
        sendCompleted(false);
    }

    void sendCompleted(bool flag) {
        if (_cb) {
            //全部发送成功或失败回调  [AUTO-TRANSLATED:6b9a9abf]
            //All send success or failure callback
            while (!_pkt_list.empty()) {
                _cb(_pkt_list.front().first, flag);
                _pkt_list.pop_front();
            }
        } else {
            _pkt_list.clear();
        }
    }

    void sendFrontSuccess() {
        if (_cb) {
            //发送成功回调  [AUTO-TRANSLATED:52759efc]
            //Send success callback
            _cb(_pkt_list.front().first, true);
        }
        _pkt_list.pop_front();
    }

protected:
    BufferList::SendResult _cb;
    List<std::pair<Buffer::Ptr, bool> > _pkt_list;
};

/////////////////////////////////////// BufferSendMmsg ///////////////////////////////////////

#if defined(__linux__) || defined(__linux)

class BufferSendMMsg : public BufferList, public BufferCallBack {
public:
    BufferSendMMsg(List<std::pair<Buffer::Ptr, bool> > list, SendResult cb);
    ~BufferSendMMsg() override = default;

    bool empty() override;
    size_t count() override;
    ssize_t send(int fd, int flags) override;

private:
    void reOffset(size_t n);
    ssize_t send_l(int fd, int flags);

private:
    size_t _remain_size = 0;
    std::vector<struct iovec> _iovec;
    std::vector<struct mmsghdr> _hdrvec;
};

bool BufferSendMMsg::empty() {
    return _remain_size == 0;
}

size_t BufferSendMMsg::count() {
    return _hdrvec.size();
}

ssize_t BufferSendMMsg::send_l(int fd, int flags) {
    ssize_t n;
    do {
        n = sendmmsg(fd, &_hdrvec[0], _hdrvec.size(), flags);
    } while (-1 == n && UV_EINTR == get_uv_error(true));

    if (n > 0) {
        //部分或全部发送成功  [AUTO-TRANSLATED:a3f5e70e]
        //partially or fully sent successfully
        reOffset(n);
        return n;
    }

    //一个字节都未发送  [AUTO-TRANSLATED:c33c611b]
    //not a single byte sent
    return n;
}

ssize_t BufferSendMMsg::send(int fd, int flags) {
    auto remain_size = _remain_size;
    while (_remain_size && send_l(fd, flags) != -1);
    ssize_t sent = remain_size - _remain_size;
    if (sent > 0) {
        //部分或全部发送成功  [AUTO-TRANSLATED:a3f5e70e]
        //partially or fully sent successfully
        return sent;
    }
    //一个字节都未发送成功  [AUTO-TRANSLATED:858b63e5]
    //not a single byte sent successfully
    return -1;
}

void BufferSendMMsg::reOffset(size_t n) {
    for (auto it = _hdrvec.begin(); it != _hdrvec.end();) {
        auto &hdr = *it;
        auto &io = *(hdr.msg_hdr.msg_iov);
        assert(hdr.msg_len <= io.iov_len);
        _remain_size -= hdr.msg_len;
        if (hdr.msg_len == io.iov_len) {
            //这个udp包全部发送成功  [AUTO-TRANSLATED:fce1cc86]
            //this UDP packet sent successfully
            it = _hdrvec.erase(it);
            sendFrontSuccess();
            continue;
        }
        //部分发送成功  [AUTO-TRANSLATED:4c240905]
        //partially sent successfully
        io.iov_base = (char *)io.iov_base + hdr.msg_len;
        io.iov_len -= hdr.msg_len;
        break;
    }
}

static inline BufferSock *getBufferSockPtr(std::pair<Buffer::Ptr, bool> &pr) {
    if (!pr.second) {
        return nullptr;
    }
    return static_cast<BufferSock *>(pr.first.get());
}
BufferSendMMsg::BufferSendMMsg(List<std::pair<Buffer::Ptr, bool>> list, SendResult cb)
    : BufferCallBack(std::move(list), std::move(cb))
    , _iovec(_pkt_list.size())
    , _hdrvec(_pkt_list.size()) {
    auto i = 0U;
    _pkt_list.for_each([&](std::pair<Buffer::Ptr, bool> &pr) {
        auto &io = _iovec[i];
        io.iov_base = pr.first->data();
        io.iov_len = pr.first->Size();
        _remain_size += io.iov_len;

        auto ptr = getBufferSockPtr(pr);
        auto &mmsg = _hdrvec[i];
        auto &msg = mmsg.msg_hdr;
        mmsg.msg_len = 0;
        msg.msg_name = ptr ? (void *)ptr->sockaddr() : nullptr;
        msg.msg_namelen = ptr ? ptr->socklen() : 0;
        msg.msg_iov = &io;
        msg.msg_iovlen = 1;
        msg.msg_control = nullptr;
        msg.msg_controllen = 0;
        msg.msg_flags = 0;
        ++i;
    });
}

#endif //defined(__linux__) || defined(__linux)

}//namespace chw


#endif//__UDP_MMSG_H