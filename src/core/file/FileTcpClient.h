#ifndef __FILE_TCP_CLIENT_H
#define __FILE_TCP_CLIENT_H

#include <memory>
#include "TcpClient.h"
#include "ComProtocol.h"
#include "EventLoop.h"

namespace chw {

/**
 * 文件传输tcp Client，发送文件。
 */
class FileTcpClient : public TcpClient {
public:
    using Ptr = std::shared_ptr<FileTcpClient>;

    FileTcpClient(const EventLoop::Ptr &poller = nullptr);
    virtual ~FileTcpClient() = default;

    // 接收数据回调（epoll线程执行）
    virtual void onRecv(const Buffer::Ptr &pBuf) override;

    // 错误回调
    virtual void onError(const SockException &ex) override;

    void StartTransf();
private:
    /**
     * @brief 处理文件发送响应
     * 
     * @param pBuf 响应数据
     */
    void procTranRsp(const Buffer::Ptr &pBuf);

    void procTranEnd(const Buffer::Ptr &pBuf);

    /**
     * @brief 文件发送结束
     * 
     * @param status 状态码(FileTranStatus)
     */
    void localTransEnd(uint32_t status);

private:
    uint32_t _status;//FileTranStatus
    EventLoop::Ptr _send_poller;
};

}//namespace chw

#endif//__FILE_TCP_CLIENT_H