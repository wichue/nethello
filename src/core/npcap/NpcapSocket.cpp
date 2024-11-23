// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "NpcapSocket.h"
#include <WinSock2.h>
#include <Windows.h>

#include "util.h"
#include "Logger.h"

#pragma comment(lib, "packet.lib")
#pragma comment(lib, "wpcap.lib")

namespace chw {

NpcapSocket::NpcapSocket()
{
    _handle = nullptr;
    _bRunning = false;
    SetReadCb(nullptr);
}

NpcapSocket::~NpcapSocket()
{
    if(_handle != nullptr)
    {
        pcap_close(_handle);
    }
}

uint32_t NpcapSocket::OpenAdapter(std::string desc)
{
	pcap_if_t *allAdapters;    // 所有网卡设备保存
	pcap_if_t *ptr;            // 用于遍历的指针

	char errbuf[PCAP_ERRBUF_SIZE];
 
    bool bMatch = false;// 是否匹配到适配器
	/* 获取本地机器设备列表 */
	if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &allAdapters, errbuf) != -1)
	{
		/* 打印网卡信息列表 */
		for (ptr = allAdapters; ptr != NULL; ptr = ptr->next)
		{
			if (ptr->description)
            {
                PrintD("adapter desc:%s",ptr->description);
                if(Contain(ptr->description,desc) == true)
                {
                    PrintD("find adapter:%s",ptr->description);
                    bMatch = true;
                    break;
                }
            }
		}
	}
    else
    {
        PrintE("pcap_findalldevs_ex failed,err:%s.",errbuf);
        return chw::fail;
    }

    if(bMatch == false)
    {
        PrintE("not match adapter:%s.",desc);
        return chw::fail; 
    }

    // PCAP_OPENFLAG_PROMISCUOUS:设置网卡混杂模式
    // PCAP_OPENFLAG_NOCAPTURE_LOCAL:不抓去自己发的包
    // PCAP_OPENFLAG_MAX_RESPONSIVENESS:设置最小拷贝为0   有数据包，立即触发回调函数
    /*
    pcap_t *pcap_open(
    const char *               source,// 要打开的适配器名称
    int                        snaplen,// 需要保存的数据包的长度，超过该长度的数据丢弃
    int                        flags,
    int                         read_timeout,// 单位毫秒，当收到一个数据包时不立即返回，而是等待该时长再返回，可以一次读取更多数据，提升性能
    struct pcap_rmtauth *       auth,// 用于远程抓包，通常设置为NULL
    char *                      errbuf// 当发生错误时保存错误信息，用户申请的缓冲区
    返回值：成功返回指向pcap_t的指针，失败返回NULL
    )
    */
    _handle = pcap_open(ptr->name, 65535, PCAP_OPENFLAG_PROMISCUOUS | PCAP_OPENFLAG_NOCAPTURE_LOCAL | PCAP_OPENFLAG_MAX_RESPONSIVENESS, 1, NULL, 0);
 
	/* 不再需要设备列表了，释放它 */
	pcap_freealldevs(allAdapters);

    if(_handle == nullptr)
    {
        PrintE("open adapter failed,err:%s.",errbuf);
        return chw::fail;
    }

    return chw::success;
}

int32_t NpcapSocket::SendToAdapter(char* buf, uint32_t len)
{
    if(_handle == nullptr)
    {
        PrintE("adapter _handle is nullptr");
        return -1;
    }

    // pcap_sendpacket返回值：成功返回0，失败返回-1
    int32_t iRet = pcap_sendpacket(_handle,(const u_char*)buf,len);
    if(iRet != 0)
    {
        PrintE("adapter send failed,iRet:%d,err:%s",iRet,pcap_geterr(_handle));
    }

    return iRet;
}

void NpcapSocket::SetReadCb(std::function<void(const u_char* buf, uint32_t len)> cb)
{
    if(cb != nullptr) {
        _cb = std::move(cb);
    } else {
        _cb = [](const u_char* buf, uint32_t len){
            PrintW("NpcapSocket not set read callback, data ignored: %d",len);
        };
    }
}

uint32_t NpcapSocket::StartRecvFromAdapter()
{
    if(_handle == nullptr)
    {
        PrintE("adapter _handle is nullptr");
        return chw::fail;
    }
    _bRunning = true;
    while(_bRunning)
    {
        pcap_pkthdr *Packet_Header = nullptr;    // 数据包头
        const u_char * Packet_Data = nullptr;    // 数据本身
        int retValue = 0;
        // pcap_next_ex返回值：成功返回接收包个数，超时返回0，发送错误返回-1
        retValue = pcap_next_ex(_handle, &Packet_Header, &Packet_Data);

        if(retValue > 0) {
            // 成功接收到数据包
            _cb(Packet_Data,Packet_Header->len);
        } else if (retValue == 0) {
            // 超时未收到数据
            continue;
        } else {
            // 发生错误
            PrintE("adapter recv failed,retValue:%d,err:%s",retValue,pcap_geterr(_handle));
            return chw::fail;
        }
    }

    return chw::success;
}

uint32_t NpcapSocket::StopRecvFromAdapter()
{
    _bRunning = false;

    return chw::success;
}


}//namespace chw 