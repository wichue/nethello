#include "UdpTextClient.h"

namespace chw {

UdpTextClient::UdpTextClient(const EventLoop::Ptr &poller) : UdpClient(poller)
{

}

UdpTextClient::~UdpTextClient()
{

}

void UdpTextClient::onRecv(const Buffer::Ptr &pBuf)
{
    //接收数据事件
    // DebugL << pBuf->data() << " from port:" << getSock()->get_peer_port();
    PrintD("<%s",pBuf->data());
    pBuf->Reset();
}

void UdpTextClient::onError(const SockException &ex)
{
    //断开连接事件，一般是EOF
    WarnL << ex.what();
}

}//namespace chw