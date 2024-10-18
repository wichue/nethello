#include "TcpClient.h"

using namespace std;

namespace chw {

// StatisticImp(TcpClient)

TcpClient::TcpClient(const EventLoop::Ptr &poller) : Client(poller)
{
    _poller = poller;
    // setPoller(poller ? poller : EventPollerPool::Instance().getPoller());//chw

    // setOnCreateSocket([](const EventLoop::Ptr &poller) {
    //     //TCP客户端默认开启互斥锁
    //     return Socket::createSocket(poller, true);
    // });
}

TcpClient::~TcpClient() {
    TraceL << "~" << TcpClient::getIdentifier();
}

// void TcpClient::shutdown(const SockException &ex) {
//     // _timer.reset();
//     // SocketHelper::shutdown(ex);
//     _sock->shutdown(ex);
// }

// bool TcpClient::alive() const {
//     // if (_timer) {
//         //连接中或已连接
//         // return true;
//     // }
//     //在websocket client()相关代码中
//     //_timer一直为空，但是socket fd有效，alive状态也应该返回true
//     // auto sock = getSock();
//     return _sock && _sock->alive();
// }

// void TcpClient::setNetAdapter(const string &local_ip) {
//     _net_adapter = local_ip;
// }
//float timeout_sec
uint32_t TcpClient::create_client(const string &url, uint16_t port, uint16_t localport, const std::string &localip) {
    float timeout_sec = 5;
    weak_ptr<TcpClient> weak_self = static_pointer_cast<TcpClient>(shared_from_this());
    // _timer = std::make_shared<Timer>(2.0f, [weak_self]() {
    //     auto strong_self = weak_self.lock();
    //     if (!strong_self) {
    //         return false;
    //     }
    //     strong_self->onManager();
    //     return true;
    // }, getPoller());

    // setSock(createSocket());
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
    // sock_ptr->setOnFlush([weak_self, sock_ptr]() {
    //     auto strong_self = weak_self.lock();
    //     if (!strong_self) {
    //         return false;
    //     }
    //     if (sock_ptr != strong_self->getSock().get()) {
    //         //已经重连socket，上传socket的事件忽略掉  [AUTO-TRANSLATED:243a8c95]
    //         //Socket has been reconnected, upload socket's event is ignored
    //         return false;
    //     }
    //     strong_self->onFlush();
    //     return true;
    // });

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

// std::string TcpClient::getIdentifier() const {
//     if (_id.empty()) {
//         static atomic<uint64_t> s_index { 0 };
//         _id = chw::demangle(typeid(*this).name()) + "-" + to_string(++s_index);
//     }
//     return _id;
// }

// uint32_t TcpClient::senddata(uint8_t* buff, uint32_t len)
// {
//     if(_sock) {
//         return _sock->send_i(buff,len);
//     } else {
//         return 0;
//     }
// }

} //namespace chw
