#ifndef __PRESS_SESSION_H
#define __PRESS_SESSION_H

#include <memory>
#include "Session.h"

namespace chw {

/**
 * 压力测试模式服务端会话，解析数据，统计接收包数量和丢包率。
 */
class PressSession : public Session {
public:
    using Ptr = std::shared_ptr<PressSession>;
    PressSession(const Socket::Ptr &sock);
    virtual ~PressSession() = default;

    // 接收数据回调（epoll线程执行）
    virtual void onRecv(const Buffer::Ptr &pBuf) override;

    // 错误回调
    virtual void onError(const SockException &ex) override;

    void onManager() override;
    const std::string &className() const override;

    /**
     * @brief tcp发送数据（可在任意线程执行）
     * 
     * @param buff 数据
     * @param len  数据长度
     * @return uint32_t 发送成功的数据长度
     */
    uint32_t senddata(uint8_t* buff, uint32_t len) override;

    /**
     * @brief 返回接收包的数量
     * 
     * @return uint64_t 接收包的数量
     */
    virtual uint64_t GetPktNum()override;

    /**
     * @brief 返回接收包的最大序列号
     * 
     * @return uint64_t 接收包的最大序列号
     */
    virtual uint64_t GetSeq()override;

    /**
     * @brief 返回接收字节总大小
     * 
     * @return uint64_t 接收字节总大小
     */
    virtual uint64_t GetRcvLen()override;

private:
    uint64_t _server_rcv_num;// 接收包的数量
    uint64_t _server_rcv_seq;// 接收包的最大序列号
    uint64_t _server_rcv_len;// 接收的字节总大小

    std::string _cls;
};

}//namespace chw

#endif//__PRESS_SESSION_H