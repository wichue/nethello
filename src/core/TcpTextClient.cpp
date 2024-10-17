#include "TcpTextClient.h"

namespace chw {

TcpTextClient::TcpTextClient(const EventLoop::Ptr &poller) : TcpClient(poller)
{

}

TcpTextClient::~TcpTextClient()
{

}

//连接结果事件
void TcpTextClient::onConnect(const SockException &ex)
{
    // InfoL << (ex ?  ex.what() : "success");
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

void TcpTextClient::onRecv(const Buffer::Ptr &pBuf)
{
    //接收数据事件
    // DebugL << pBuf->data() << " from port:" << getSock()->get_peer_port();
    PrintD("<%s",pBuf->data());
    pBuf->Reset();
}

void TcpTextClient::onError(const SockException &ex)
{
    //断开连接事件，一般是EOF
    WarnL << ex.what();
}

const Socket::Ptr &TcpTextClient::getSock() const
{
    return _sock;
}

}//namespace chw