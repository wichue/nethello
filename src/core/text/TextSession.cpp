#include "TextSession.h"

namespace chw {

TextSession::TextSession(const Socket::Ptr &sock) :Session(sock)
{
    _cls = chw::demangle(typeid(TextSession).name());
}

TextSession::~TextSession()
{

}

/**
 * @brief 接收数据回调（epoll线程执行）
 * 
 * @param buf [in]数据
 */
void TextSession::onRecv(const Buffer::Ptr &buf)
{
    PrintD("<%s",buf->data());
    buf->Reset0();
}


/**
 * @brief 发生错误时的回调
 * 
 * @param err [in]异常
 */
void TextSession::onError(const SockException &err)
{

}

/**
 * @brief 定时器周期管理回调
 * 
 */
void TextSession::onManager()
{

}

/**
 * @brief 返回当前类名称
 * 
 * @return const std::string& 类名称
 */
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