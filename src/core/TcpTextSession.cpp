#include "TcpTextSession.h"

namespace chw {

TcpTextSession::TcpTextSession(const Socket::Ptr &sock) :Session(sock)
{
    _cls = chw::demangle(typeid(TcpTextSession).name());
}

TcpTextSession::~TcpTextSession()
{

}

void TcpTextSession::onRecv(const Buffer::Ptr &buf)
{

}

void TcpTextSession::onError(const SockException &err)
{

}

void TcpTextSession::onManager()
{

}

const std::string &TcpTextSession::className() const {
    return _cls;
}

}//namespace chw