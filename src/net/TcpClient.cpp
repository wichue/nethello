#include "TcpClient.h"

using namespace std;

namespace chw {

TcpClient::TcpClient(const EventLoop::Ptr &poller) : Client(poller)
{
    _poller = poller;
}

TcpClient::~TcpClient() {
    TraceL << "~" << TcpClient::getIdentifier();
}

uint32_t TcpClient::create_client(const string &url, uint16_t port, uint16_t localport, const std::string &localip) {
    float timeout_sec = 5;
    weak_ptr<TcpClient> weak_self = static_pointer_cast<TcpClient>(shared_from_this());
    _socket = Socket::createSocket(_poller);

    auto sock_ptr = _socket.get();
    sock_ptr->setOnErr([weak_self, sock_ptr](const SockException &ex) {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }
        if (sock_ptr != strong_self->_socket.get()) {
            //已经重连socket，上次的socket的事件忽略掉
            return;
        }
        // strong_self->_timer.reset();
        TraceL << strong_self->getIdentifier() << " on err: " << ex;
        strong_self->onError(ex);
    });

    TraceL << getIdentifier() << " start connect " << url << ":" << port;
    sock_ptr->connect(url, port, [weak_self](const SockException &err) {
        auto strong_self = weak_self.lock();
        if (strong_self) {
            strong_self->onSockConnect(err);
        }
    }, timeout_sec, localip, localport);

    return chw::success;
}

void TcpClient::onSockConnect(const SockException &ex) {
    // TraceL << getIdentifier() << " connect result: " << ex;//chw
    if (ex) {
        //连接失败
        // _timer.reset();
        onConnect(ex);
        return;
    }

    auto sock_ptr = _socket.get();
    weak_ptr<TcpClient> weak_self = static_pointer_cast<TcpClient>(shared_from_this());

    sock_ptr->setOnRead([weak_self, sock_ptr](const Buffer::Ptr &pBuf, struct sockaddr *, int) {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }
        if (sock_ptr != strong_self->_socket.get()) {
            //已经重连socket，上传socket的事件忽略掉
            return;
        }
        try {
            strong_self->onRecv(pBuf);
        } catch (std::exception &ex) {
            strong_self->shutdown(SockException(Err_other, ex.what()));
        }
    });

    onConnect(ex);
}

void TcpClient::onConnect(const SockException &ex)
{
    if(ex)
    {
        PrintE("tcp connect failed, please check ip and port, ex:%s.", ex.what());
        sleep_exit(100*1000);
    }
    else
    {
        PrintD("connect success.");
        usleep(1000);
        InfoLNCR << ">";
    }
}

} //namespace chw
