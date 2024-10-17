#include "UdpClient.h"
#include "util.h"

using namespace std;

namespace chw {

// StatisticImp(UdpClient)

UdpClient::UdpClient(const EventLoop::Ptr &poller) {
    _poller = poller;
    // setPoller(poller ? poller : EventPollerPool::Instance().getPoller());//chw

    // setOnCreateSocket([](const EventLoop::Ptr &poller) {
    //     //TCP客户端默认开启互斥锁
    //     return Socket::createSocket(poller, true);
    // });
}

UdpClient::~UdpClient() {
    TraceL << "~" << UdpClient::getIdentifier();
}

void UdpClient::shutdown(const SockException &ex) {
    _timer.reset();
    // SocketHelper::shutdown(ex);
    _sock->shutdown(ex);
}

bool UdpClient::alive() const {
    if (_timer) {
        //连接中或已连接
        return true;
    }
    //在websocket client()相关代码中
    //_timer一直为空，但是socket fd有效，alive状态也应该返回true
    // auto sock = getSock();
    return _sock && _sock->alive();
}

uint32_t UdpClient::create_udp(const std::string &url, uint16_t port,const std::string &localip, uint16_t localport)
{
    weak_ptr<UdpClient> weak_self = static_pointer_cast<UdpClient>(shared_from_this());
    _sock = Socket::createSocket(_poller);

    auto sock_ptr = _sock.get();
    sock_ptr->setOnErr([weak_self, sock_ptr](const SockException &ex) {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }
        if (sock_ptr != strong_self->_sock.get()) {
            //已经重连socket，上次的socket的事件忽略掉
            return;
        }
        strong_self->_timer.reset();
        TraceL << strong_self->getIdentifier() << " on err: " << ex;
        strong_self->onError(ex);
    });

    //udp绑定本地IP和端口
    if (!sock_ptr->bindUdpSock(localport, localip.c_str()))
    {
        // udp 绑定端口失败, 可能是由于端口占用或权限问题
        PrintE("Bind udp socket failed, local ip=%s,port=%u,err=%s",localip.c_str(),localport,get_uv_errmsg(true));

        return chw::fail;
    }

    //udp绑定远端IP和端口
    struct sockaddr_in local_addr;
    local_addr.sin_addr.s_addr = inet_addr(url.c_str());
    local_addr.sin_port = htons(port);
    local_addr.sin_family = AF_INET;
    if(!sock_ptr->bindPeerAddr((struct sockaddr *) &local_addr, sizeof(struct sockaddr_in),true))
    {
        PrintE("bind peer addr failed, remote ip=%s, port=%u",url.c_str(),port);

        return chw::fail;
    }

    sock_ptr->setOnRead([weak_self, sock_ptr](const Buffer::Ptr &pBuf, struct sockaddr_storage *, int) {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }
        if (sock_ptr != strong_self->_sock.get()) {
            //已经重连socket，上传socket的事件忽略掉
            return;
        }
        try {
            strong_self->onRecv(pBuf);
        } catch (std::exception &ex) {
            strong_self->shutdown(SockException(Err_other, ex.what()));
        }
    });

    return chw::success;
}

std::string UdpClient::getIdentifier() const {
    if (_id.empty()) {
        static atomic<uint64_t> s_index { 0 };
        _id = chw::demangle(typeid(*this).name()) + "-" + to_string(++s_index);
    }
    return _id;
}

uint32_t UdpClient::senddata(uint8_t* buff, uint32_t len)
{
    if(_sock) {
        return _sock->send_i(buff,len);
    } else {
        return 0;
    }
}

} //namespace chw
