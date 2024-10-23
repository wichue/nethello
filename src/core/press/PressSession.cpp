#include "PressSession.h"
#include "ComProtocol.h"

namespace chw {

PressSession::PressSession(const Socket::Ptr &sock) : Session(sock)
{
    _cls = chw::demangle(typeid(PressSession).name());

    _server_rcv_num = 0;
    _server_rcv_seq = 0;
    _server_rcv_len = 0;
}

void PressSession::onRecv(const Buffer::Ptr &pBuf)
{
    if(getSock()->sockType() == SockNum::Sock_UDP)
    {
        MsgHdr* pMsgHdr = (MsgHdr*)pBuf->data();
        if(_server_rcv_seq < pMsgHdr->uMsgIndex)
        {
            _server_rcv_seq = pMsgHdr->uMsgIndex;
        }
    }

    _server_rcv_num ++;
    _server_rcv_len += pBuf->RcvLen();

    pBuf->Reset();
}

void PressSession::onError(const SockException &ex)
{
    //断开连接事件，一般是EOF
    WarnL << ex.what();
}

void PressSession::onManager()
{

}

const std::string &PressSession::className() const 
{
    return _cls;
}

/**
 * @brief 发送数据
 * 
 * @param buff 数据
 * @param len  数据长度
 * @return uint32_t 发送成功的数据长度
 */
uint32_t PressSession::senddata(uint8_t* buff, uint32_t len)
{
    if(getSock()) {
        return getSock()->send_i(buff,len);
    } else {
        PrintE("tcp session already disconnect");
        return 0;
    }
}

uint64_t PressSession::GetPktNum()
{
    return _server_rcv_num;
}

uint64_t PressSession::GetSeq()
{
    return _server_rcv_seq;
}

uint64_t PressSession::GetRcvLen()
{
    return _server_rcv_len;
}

}//namespace chw