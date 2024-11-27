// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __NPCAP_SOCKET_H
#define __NPCAP_SOCKET_H

#include <stdint.h>
#include <string>
#include <functional>
#include <memory>
#include <pcap.h>

namespace chw {

/**
 * @brief https://npcap.com/#download
 * 下载：npcap-sdk-1.13.zip，解压缩后获取Lib和Include文件夹。
 * VS-属性-C/C++-附加包含目录，把...\Include目录添加进去。
 * VS-属性-调试-环境，添加 PATH=...\Lib;%PATH%
 * VS-属性-连接器-常规-附加库目录，添加 ...\Lib
 * ...用完整目录替代。
 * 如果提示缺少Packet.dll和wpcap.dll，拷贝适合当前操作系统环境的库到可执行文件目录。
 */
class NpcapSocket : public std::enable_shared_from_this<NpcapSocket>
{
public:
    using Ptr = std::shared_ptr<NpcapSocket>;
    NpcapSocket();
    ~NpcapSocket();

    /**
     * @brief 打开网络适配器
     * 
     * @param desc      适配器描述，包含唯一关键字段即可
     * @return uint32_t 成功返回chw::success,失败返回chw::fail
     */
    uint32_t OpenAdapter(std::string desc);

    /**
     * @brief 从网络适配器发送数据，自动加入以太头
     * 
     * @param buf 数据
     * @param len 数据长度
     * @return int32_t 发送成功返回0，失败返回-1
     */
    int32_t SendToAdapter(char* buf, uint32_t len);

    /**
     * @brief 从网络适配器发送数据，默认已经加入以太头，直接发送数据
     * 
     * @param buf 数据
     * @param len 数据长度
     * @return int32_t 发送成功返回0，失败返回-1
     */
    int32_t SendToAdPure(char* buf, uint32_t len);

    /**
     * @brief 开始从网卡适配器接收数据
     * 
     * @return uint32_t 正常结束返回chw::success,发生错误返回chw::fail
     */
    uint32_t StartRecvFromAdapter();

    /**
     * @brief 停止从网卡适配器接收数据
     * 
     * @return uint32_t chw::success
     */
    uint32_t StopRecvFromAdapter();

    /**
     * @brief 设置读数据回调
     * 
     * @param cb 回调函数
     */
    void SetReadCb(std::function<void(const u_char* buf, uint32_t len)> cb);

private:
    pcap_t * _handle;// 网络适配器句柄
    bool _bRunning;// 是否循环接收数据
    std::function<void(const u_char*, uint32_t len)> _cb;// 读数据回调
public:
    uint8_t _local_mac[6] = { 0 };// 本端绑定网卡的MAC地址
};

}//namespace chw 



#endif//__NPCAP_SOCKET_H