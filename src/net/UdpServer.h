// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __UDP_SERVER_H
#define __UDP_SERVER_H

#include <functional>
#include <unordered_map>
#include "Session.h"
#include "Server.h"

namespace chw {

class UdpServer : public Server {
public:
    using Ptr = std::shared_ptr<UdpServer>;
    using PeerIdType = std::string;

    explicit UdpServer(const EventLoop::Ptr &poller = nullptr);
    ~UdpServer();

    /**
     * @brief 获取会话接收信息
     * 
     * @param rcv_num   [out]接收包的数量
     * @param rcv_seq   [out]接收包的最大序列号
     * @param rcv_len   [out]接收的字节总大小
     * @param rcv_speed [out]接收速率
     */
    virtual void GetRcvInfo(uint64_t& rcv_num,uint64_t& rcv_seq,uint64_t& rcv_len,uint64_t& rcv_speed) override;

private:
    /**
     * @brief 开始udp server
     * @param port [in]本机端口，0则随机
     * @param host [in]监听网卡ip
     */
    virtual void start_l(uint16_t port, const std::string &host = "::") override;

    /**
     * @brief 定时管理 Session, UDP 会话需要根据需要处理超时
     */
    virtual void onManagerSession() override;

    /**
     * @brief udp服务端Socket接收回调
     * 
     * @param buf       [in]数据
     * @param addr      [in]对端地址
     * @param addr_len  [in]对端地址长度
     */
    void onRead(Buffer::Ptr &buf, struct sockaddr *addr, int addr_len);

    /**
     * @brief udp服务端Socket接收到数据,获取或创建session,消息可能来自server fd，也可能来自peer fd
     * @param is_server_fd  [in]是否为server fd
     * @param id            [in]客户端id
     * @param buf           [in]数据
     * @param addr          [in]客户端地址
     * @param addr_len      [in]客户端地址长度
     */
    void onRead_l(bool is_server_fd, const PeerIdType &id, Buffer::Ptr &buf, struct sockaddr *addr, int addr_len);

    /**
     * @brief 根据对端信息获取或创建一个会话
     * 
     * @param id        [in]会话唯一标识
     * @param buf       [in]buf
     * @param addr      [in]对端地址
     * @param addr_len  [in]对端地址长度
     * @param is_new    [out]是否新接入连接
     * @return Session::Ptr 会话
     */
    Session::Ptr getOrCreateSession(const PeerIdType &id, Buffer::Ptr &buf, struct sockaddr *addr, int addr_len, bool &is_new);

    /**
     * @brief 创建一个会话, 同时进行必要的设置
     * 
     * @param id        [in]会话唯一标识
     * @param buf       [in]buf
     * @param addr      [in]对端地址
     * @param addr_len  [in]对端地址长度
     * @return Session::Ptr 会话
     */
    Session::Ptr createSession(const PeerIdType &id, Buffer::Ptr &buf, struct sockaddr *addr, int addr_len);

    /**
    * @brief 创建服务端Socket，设置接收回调
    * 
    */
    void setupEvent();

private:
    std::shared_ptr<Timer> _timer;
    std::shared_ptr<std::recursive_mutex> _session_mutex;

//chw
public:
    /**
     * @brief 发送数据给最后一个活动的客户端
     * 
     * @param buf   [in]数据
     * @param len   [in]数据长度
     * @return uint32_t 发送成功的数据长度
     */
    virtual uint32_t sendclientdata(uint8_t* buf, uint32_t len) override;
    std::weak_ptr<Session> _last_session;// 最后一个活动的客户端
private:
    std::shared_ptr<std::unordered_map<PeerIdType, Session::Ptr> > _session_map;
};

} // namespace chw

#endif // __UDP_SERVER_H
