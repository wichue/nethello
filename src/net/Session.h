#ifndef __SESSION_H
#define __SESSION_H

#include <memory>
#include <atomic>
#include "Socket.h"

namespace chw {
    
class Session : public std::enable_shared_from_this<Session> {
public:
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

private:
    mutable std::string _id;
    Socket::Ptr _sock;
    std::string _cls;
};

} // namespace chw

#endif//__SESSION_H