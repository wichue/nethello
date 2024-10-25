#include "TextSession.h"

namespace chw {

TextSession::TextSession(const Socket::Ptr &sock) :Session(sock)
{
    _cls = chw::demangle(typeid(TextSession).name());
}

TextSession::~TextSession()
{

}

void TextSession::onRecv(const Buffer::Ptr &buf)
{
    PrintD("<%s",buf->data());
    buf->Reset0();
}

void TextSession::onError(const SockException &err)
{

}

void TextSession::onManager()
{

}

const std::string &TextSession::className() const 
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
uint32_t TextSession::senddata(uint8_t* buff, uint32_t len)
{
    if(getSock()) {
        return getSock()->send_i(buff,len);
    } else {
        PrintE("tcp session already disconnect");
        return 0;
    }
}

}//namespace chw