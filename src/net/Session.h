#ifndef __SESSION_H
#define __SESSION_H

#include <memory>
#include <atomic>
#include "Socket.h"

namespace chw {
/**
 * tcp/udp服务端会话基类，继承该类可实现自定义服务端会话。
 * 
 */
class Session : public std::enable_shared_from_this<Session> {
public:
    bool enable = true;
    using Ptr = std::shared_ptr<Session>;
    Session(const Socket::Ptr &sock)
    {
        _sock = sock;
    };

    ~Session() = default;

    std::string getIdentifier() const {
        if (_id.empty()) {
            static std::atomic<uint64_t> s_session_index{0};
            _id = std::to_string(++s_session_index) + '-' + std::to_string(getSock()->rawFD());
        }
        return _id;
    }

    const Socket::Ptr &getSock() const
    {
        return _sock;
    }

    virtual void onRecv(const Buffer::Ptr &buf) = 0;
    virtual void onError(const SockException &err) = 0;
    virtual void onManager() = 0;
    virtual const std::string &className() const = 0;
    virtual uint32_t senddata(uint8_t* buff, uint32_t len) = 0;

    virtual uint64_t GetPktNum() = 0;
    virtual uint64_t GetSeq() = 0;
    virtual uint64_t GetRcvLen() = 0;

private:
    mutable std::string _id;
    Socket::Ptr _sock;
    std::string _cls;
};

} // namespace chw

#endif//__SESSION_H