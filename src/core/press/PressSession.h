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

    /**
     * @brief 接收数据回调（epoll线程执行）
     * 
     * @param buf [in]数据
     */
    virtual void onRecv(const Buffer::Ptr &pBuf) override;

    /**
     * @brief 发生错误时的回调
     * 
     * @param err [in]异常
     */
    virtual void onError(const SockException &ex) override;

    /**
     * @brief 定时器周期管理回调
     * 
     */
    void onManager() override;

    /**
     * @brief 返回当前类名称
     * 
     * @return const std::string& 类名称
     */
    const std::string &className() const override;

    /**
     * @brief 发送数据（可在任意线程执行）
     * 
     * @param buff [in]数据
     * @param len  [in]数据长度
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
    uint64_t _server_rcv_seq;// 接收包的最大序列号,注意udp可能乱序
    uint64_t _server_rcv_len;// 接收的字节总大小

    std::string _cls;
};

}//namespace chw

#endif//__PRESS_SESSION_H