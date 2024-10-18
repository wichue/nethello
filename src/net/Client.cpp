#include "Client.h"
#include <atomic>

namespace chw {

Client::Client(const EventLoop::Ptr &poller) : _poller(poller)
{

}

void Client::shutdown(const SockException &ex)
{
    _socket->shutdown(ex);
}

bool Client::alive() const
{
    return _socket && _socket->alive();
}

std::string Client::getIdentifier() const
{
    if (_id.empty()) {
        static std::atomic<uint64_t> s_index { 0 };
        _id = chw::demangle(typeid(*this).name()) + "-" + std::to_string(++s_index);
    }
    return _id;
}

const Socket::Ptr &Client::getSock() const
{
    return _socket;
}

uint32_t Client::senddata(uint8_t* buff, uint32_t len)
{
    if(_socket) {
        return _socket->send_i(buff,len);
    } else {
        return 0;
    }
}

}//namespace chw 