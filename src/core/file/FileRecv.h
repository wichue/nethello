// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __FILE_RECV_H
#define __FILE_RECV_H

#include <memory>
#include "Session.h"

namespace chw {

/**
 * 文件传输接收端封装，只关注文件传输的信令和数据交互，不关注数据如何发送和接收。
 * 支持tcp、upd+kcp、raw+kcp等多种模式。
 * 使用时调用SetSndData设置发送函数，把接收的数据传递给onRecv。
 */
class FileRecv : public std::enable_shared_from_this<FileRecv> {
public:
    using Ptr = std::shared_ptr<FileRecv>();
    FileRecv();
    ~FileRecv();

    void SetSndData(std::function<uint32_t(char* buf, uint32_t )> cb);

    /**
     * @brief 接收数据回调（epoll线程执行）
     * 
     * @param buf [in]数据
     */
    void onRecv(const Buffer::Ptr &buf);

    /**
     * @brief 发生错误时的回调
     * 
     * @param err [in]异常
     */
    void onError(const SockException &err);

    /**
     * @brief 定时器周期管理回调
     * 
     */
    void onManager();

    /**
     * @brief 返回当前类名称
     * 
     * @return const std::string& 类名称
     */
    const std::string &className() const;
private:
    /**
     * @brief 接收传输结束消息
     * 
     * @param buf [in]数据
     */
    void procTranEnd(char* buf);

    /**
     * @brief 接收文件传输请求消息
     * 
     * @param buf [in]数据
     */
    void procTranReq(char* buf);

    /**
     * @brief 接收文件传输数据
     * 
     * @param buf [in]数据
     * @param len [in]长度
     */
    void procFileData(char* buf, uint32_t len);

    /**
     * @brief 发送信令消息给对端
     * 
     * @param msgtype [in]消息类型
     * @param code    [in]错误码
     */
    void SendSignalMsg(uint32_t msgtype, uint32_t code);

    /**
     * @brief 分发消息
     * 
     * @param buf [in]消息
     * @param len [in]长度
     */
    void DispatchMsg(char* buf, uint32_t len);
private:
    std::string _cls;// 类名
    uint32_t _status;//FileTranStatus

    std::string _filepath;// 文件保存路经
    std::string _filename;// 文件保存文件名
    uint32_t _filesize; // 文件大小

    FILE* _write_file;// 写文件句柄
    uint32_t _write_size;// 已写入的大小
    Ticker _ticker;// 统计耗时

    /**
     * @brief 发送数据
     * 
     * @param buff [in]数据
     * @param len  [in]数据长度
     * @return uint32_t 发送成功的数据长度
     */
    std::function<uint32_t(char*, uint32_t )> _send_data;// 发送数据函数
};

}//namespace chw

#endif//__FILE_RECV_H