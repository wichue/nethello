// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "NpcapSocket.h"
#include <WinSock2.h>
#include <Windows.h>

#include "util.h"
#include "Logger.h"
#include "GetMacAddress.h"
#include "GlobalValue.h"
#include "MemoryHandle.h"
#include "config.h"

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

    if (_queue != nullptr)
    {
        pcap_sendqueue_destroy(_queue);
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
                //PrintD("adapter desc:%s,name:%s",ptr->description,ptr->name);
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

    if (GetMac(_local_mac, ptr->name) != 0)
    {
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
 
    if(_handle == nullptr)
    {
        PrintE("open adapter failed,err:%s.",errbuf);
        /* 不再需要设备列表了，释放它 */
        pcap_freealldevs(allAdapters);

        return chw::fail;
    }

    /* 不再需要设备列表了，释放它 */
    pcap_freealldevs(allAdapters);

    //设置内核缓冲区和用户缓冲区的大小
    pcap_setbuff(_handle, 200 * 1024 * 1024);
    pcap_set_buffer_size(_handle, 200 * 1024 * 1024);

    _queue = pcap_sendqueue_alloc((2000 + sizeof(struct pcap_pkthdr)) * NPCAP_SEND_QUEUE_SIZE);
    if (_queue == NULL)
    {
        PrintE("alloc npcap send queue failed.");
        pcap_close(_handle);
        return chw::fail;
    }

    PrintD("create npcap socket, local interface:%s, local mac:%s.", ptr->description, MacBuftoStr(_local_mac).c_str());

    return chw::success;
}

int32_t NpcapSocket::SendToAdapter(char* buf, uint32_t len)
{
    if(_handle == nullptr)
    {
        PrintE("SendToAdapter, adapter _handle is nullptr");
        return -1;
    }

    char* snd_buf = (char*)_RAM_NEW_(sizeof(ethhdr) + len);
    memcpy(snd_buf + sizeof(ethhdr), buf, len);

    ethhdr* peth = (ethhdr*)snd_buf;
    memcpy(peth->h_dest, gConfigCmd.dstmac, 6);
    memcpy(peth->h_source, _local_mac, 6);
    peth->h_proto = htons(ETH_RAW_TEXT);

    // pcap_sendpacket返回值：成功返回0，失败返回-1
    int32_t iRet = pcap_sendpacket(_handle,(const u_char*)snd_buf, sizeof(ethhdr) + len);
    if(iRet != 0)
    {
        PrintE("adapter send failed,iRet:%d,err:%s",iRet,pcap_geterr(_handle));
    }

    return iRet;
}

/**
 * @brief 从网络适配器发送数据，默认已经加入以太头，直接发送数据
 * 
 * @param buf 数据
 * @param len 数据长度
 * @return int32_t 发送成功返回0，失败返回-1
 */
int32_t NpcapSocket::SendToAdPure(char* buf, uint32_t len)
{
    if (_handle == nullptr)
    {
        PrintE("SendToAdPure, adapter _handle is nullptr");
        return -1;
    }

    // pcap_sendpacket返回值：成功返回0，失败返回-1
    int32_t iRet = pcap_sendpacket(_handle,(const u_char*)buf, len);
    if(iRet != 0)
    {
        PrintE("adapter send failed,iRet:%d,err:%s",iRet,pcap_geterr(_handle));
    }

    return iRet;
}

uint32_t NpcapSocket::PushNpcapSendQue(char* buf, uint32_t len)
{
    struct pcap_pkthdr PktHdr;
    PktHdr.caplen = len;
    PktHdr.len = len;
    if (pcap_sendqueue_queue(_queue, &PktHdr, (const u_char*)buf) == -1)
    {
        PrintE("npcap push queue failed,err:%s.", pcap_geterr(_handle));
        return chw::fail;
    }

    return chw::success;
}

void NpcapSocket::ClearNpcapSendQue()
{
    memset(_queue->buffer, 0, _queue->maxlen);
    _queue->len = 0;
}

uint32_t NpcapSocket::SendQueToAdPure()
{
    if (_handle == nullptr)
    {
        PrintE("SendQueToAdPure adapter _handle is nullptr");
        return 0;
    }

    /**
     * @brief pcap_sendqueue_transmit 比用pcap_sendpacket()来发送一系列的数据要高效的多，因为他的数据是在内核级上被缓冲。
     * @param p     pcap_t句柄
     * @param queue 发送队列
     * @param sync  0表示异步发送，函数立即返回，数据在后台发送；非0则同步发送，函数会阻塞直到数据包都发送完成。
     * @return  sync是0返回拷贝到内核数据大小，sync非0返回发送成功数据大小，失败返回0.
     */
    uint32_t uiRet = pcap_sendqueue_transmit(_handle, _queue, 0);
    if (uiRet <= 0)
    {
        ErrorL << "pcap_sendqueue_transmit failed,iRet=" << uiRet << ",err=" << pcap_geterr(_handle);
    }

    return uiRet;
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