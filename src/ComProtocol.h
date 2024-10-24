// Copyright (c) 2024 The idump project authors. SPDX-License-Identifier: MIT.
// This file is part of idump(https://github.com/wichue/idump).

#ifndef __COMMON_PROTOCOL_H
#define __COMMON_PROTOCOL_H

#include <stdint.h>
#include <string>
#include <netinet/in.h>// for in_addr in6_addr
#include <vector>
#include "Socket.h"

namespace chw {

#ifndef ETH_ALEN
#define ETH_ALEN    6
#endif

#pragma pack(1)
//pacp文件头结构体
struct pcap_file_header
{
    uint32_t magic;       /* 0xa1b2c3d4 */
    uint16_t version_major;   /* magjor Version 2 */
    uint16_t version_minor;   /* magjor Version 4 */
    uint32_t thiszone;      /* gmt to local correction */
    uint32_t sigfigs;     /* accuracy of timestamps */
    uint32_t snaplen;     /* max length saved portion of each pkt */
    uint32_t linktype;    /* data link type (LINKTYPE_*) */
};

//时间戳
struct time_val
{
    int tv_sec;         /* seconds 含义同 time_t 对象的值 */
    int tv_usec;        /* and microseconds */
};

//pcap数据包头结构体
struct pcap_pkthdr
{
    struct time_val ts;  /* time stamp */
    uint32_t caplen; /* length of portion present 实际捕获包的长度*/
    uint32_t len;    /* length this packet (off wire) 包的长度*/
};

// 以太头
struct ethhdr {
	unsigned char	h_dest[ETH_ALEN];	/* destination eth addr	*/
	unsigned char	h_source[ETH_ALEN];	/* source ether addr	*/
	uint16_t		h_proto;		/* packet type ID field	*/
} __attribute__((packed));

/*
 *  * IPV4头，最小20字节
 *  *
 *  0      3 4     7 8            15 16    19                         31
 *  +-------+-------+---------------+----------------------------------+
 *  |VER(4b)|LEN(4b)|  TOS(8b)      |       TOTAL LEN(16bit)           |
 *  +-------+-------+---------------+-----+----------------------------+ 4B
 *  |       identifier 16bit        |FLAGS|      OFFSET (13bit)        |
 *  +---------------+---------------+-----+----------------------------+ 8B
 *  |    TTL 8bit   | protocol 8bit |     checksum 16bit               |
 *  +---------------+---------------+----------------------------------+ 12B
 *  |                  32bit src IP                                    |
 *  +------------------------------------------------------------------+ 16B
 *  |                  32bit dst IP                                    |
 *  +------------------------------------------------------------------+ 20B
 *  \                  OPTION (if has)                                 /
 *  /                                                                  \
 *  +------------------------------------------------------------------+
 *  |                                                                  |
 *  |                     DATA...                                      |
 *  +------------------------------------------------------------------+
 *  */
// ipv4头
struct ip4hdr {
#if defined(__LITTLE_ENDIAN)
	uint8_t	ihl:4,//首部长度(4位),表示IP报文头部按32位字长（32位，4字节）计数的长度，也即报文头的长度等于IHL的值乘以4。
			version:4;//版本(4位)
#elif defined(__BIG_ENDIAN)
	uint8_t	version:4;//版本(4位)
			ihl:4,//首部长度(4位),表示IP报文头部按32位字长（32位，4字节）计数的长度，也即报文头的长度等于IHL的值乘以4。
#endif
	uint8_t	tos;// 服务类型字段(8位)
	uint16_t	tot_len;//总长度字段(16位)是指整个IP数据报的长度
	uint16_t	id;//标识，用于分片处理，同一数据报的分片具有相同的标识
	uint16_t	frag_off;//分段偏移
	uint8_t	ttl;//TTL
	uint8_t	protocol;//协议字段
	uint16_t	check;//首部校验和字段
	uint32_t	saddr;//32源IP地址
	uint32_t	daddr;//32位目的IP地址
	/*The options start here. */
};

/*  IPV6头，固定长度40字节
 *
 *  0      3 4     7 8   11 12    15 16                               31
 *  +-------+-------+------+-------------------------------------------+
 *  |ver(4b)|  TYPE(8bit)  |      stream  tag (20bit)                  |
 *  +-------+--------------+--------+----------------+-----------------+ 4B
 *  |    payload len (16bit)        |next head (8bit)|  jump limit (8b)|
 *  +-------------------------------+----------------+-----------------+ 8B
 *  |                   src IPV6 addr (128bit)                         |
 *  +------------------------------------------------------------------+ 12B
 *  |                     ..............                               |
 *  +------------------------------------------------------------------+ 16B
 *  |                     ..............                               |
 *  +------------------------------------------------------------------+ 20B
 *  |                     ..............                               |
 *  +------------------------------------------------------------------+ 24B
 *  |                   dst IPV6 addr (128bit)                         |
 *  +------------------------------------------------------------------+ 28B
 *  |                     ..............                               |
 *  +------------------------------------------------------------------+ 32B
 *  |                     ..............                               |
 *  +------------------------------------------------------------------+ 36B
 *  |                     ..............                               |
 *  +------------------------------------------------------------------+ 40B
 *  */
// ipv6头
typedef struct _IP6Hdr
{
#if defined(__LITTLE_ENDIAN)
    uint32_t flow_lbl:20;// 流标签，可用来标记报文的数据流类型，以便在网络层区分不同的报文。
    uint32_t priority:8;// 通信优先级
    uint32_t version:4;// ip版本，6
#elif defined(__BIG_ENDIAN)
    uint32_t version:4;// ip版本，6
    uint32_t priority:8;// 通信优先级
    uint32_t flow_lbl:20;// 流标签，可用来标记报文的数据流类型，以便在网络层区分不同的报文。
#endif
    uint16_t payload_len; // PayLoad Length 除了ipv6头部以外的负载长度(传输层+应用层长度)
    uint8_t nexthdr; // Next Header 可能是tcp/udp协议类型，也可能是IPv6扩展报头
    uint8_t hop_limit; // Hop Limit 跳数限制
    uint8_t saddr[16];// 源IP地址
    uint8_t daddr[16];// 目的IP地址
} ip6hdr;

// ipv6扩展头
typedef struct _IP6ExtenHdr
{
    uint8_t Ex_Next_Hdr;
    uint8_t Ex_Hdr_len;
    uint16_t reserve16;
    uint32_t reserve32;
}IP6ExtenHdr;

/*
 * udp头，长度固定8字节
 *
	0                       15 16                     31
	+-------------------------+------------------------+
	|     src Port 16bit      |   dst Port 16bit       |
	+-------------------------+------------------------+
	|     data len 16bit      |   checksum 16bit       |
	+-------------------------+------------------------+
*/
struct udphdr {
	uint16_t	source;//源端口号
	uint16_t	dest;//目的端口号
	uint16_t	len;//整个UDP数据报的长度 = 报头+载荷。
	uint16_t	check;//检测UDP数据(包含头部和数据部分)报在传输中是否有错，有错则丢弃
};

/*tcp头，最小20字节
 *
   0                   1                   2                   3   
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          Source Port(16bit)   |       Destination Port(16bit) |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Sequence Number(32bit)                 |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Acknowledgment Number(32bit)               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Data |           |U|A|P|R|S|F|                               |
   | Offset| Reserved  |R|C|S|S|Y|I|            Window(16bit)      |
   |  4bit |  16bit    |G|K|H|T|N|N|                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Checksum 16bit      |         Urgent Pointer 16bit  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Options                    |    Padding    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             data                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
struct tcphdr {
	uint16_t	source;//16位源端口
	uint16_t	dest;//16位目的端口
	uint32_t	seq;//序列号
	uint32_t	ack_seq;//确认号
#if defined(__LITTLE_ENDIAN)
	uint16_t	res1:4,// 保留位
		doff:4,//TCP头长度，指明了TCP头部包含了多少个32位的字
		fin:1,//释放一个连接，它表示发送方已经没有数据要传输了。
		syn:1,//同步序号，用来发送一个连接。syn被用于建立连接的过程。
		rst:1,//该位用于重置一个混乱的连接，之所以混乱，可能是因为主机崩溃或者其他原因。
		psh:1,
		ack:1,//ack位被设置为1表示tcphdr->ack_seq是有效的，如果ack为0，则表示该数据段不包含确认信息
		urg:1,//紧急指针有效
		ece:1,
		cwr:1;
#elif defined(__BIG_ENDIAN)
 	__u16	doff:4,
 		res1:4,
 		cwr:1,
 		ece:1,
 		urg:1,
 		ack:1,
 		psh:1,
 		rst:1,
 		syn:1,
 		fin:1;
#endif	
	uint16_t	window;//16位滑动窗口大小，单位为字节，起始于确认序号字段指明的值，这个值是接收端期望接收的字节数，其最大值为63353字节。
	uint16_t	check;//校验和，覆盖了整个tcp报文端，是一个强制性的字段，一定是由发送端计算和存储，并由接收端进行验证。
	uint16_t	urg_ptr;//  这个域被用来指示紧急数据在当前数据段中的为止，它是一个相当于当前序列号的字节偏移量。这个设置可以代替中断信息。
};

typedef enum {
    IPV4 = 4,
    IPV6 = 6,
    IP_NULL
} _IPTYPE_;

typedef enum {
    tcp_trans,
    udp_trans,
    null_trans
} _TRANSPORT_;

#pragma pack()

// 文件传输状态
enum FileTranStatus
{
    TRANS_INIT,
    TRANS_CONNECTED,// 已经建立连接,协商中
    TRANS_SENDING,  // 传输中
    TRANS_SEND_OVER,// 发送完成
    TRANS_RECV_OVER,// 接收完成
    TRANS_SEND_FAIL,// 发送失败
    TRANS_RECV_FAIL,// 接收失败
};

enum WorkModel
{
    TEXT_MODEL,     // 文本聊天模式(-T)
    PRESS_MODEL,    // 压力测试模式(-P)
    FILE_MODEL      // 文件传输模式(-F)
};

class workmodel  : public std::enable_shared_from_this<workmodel>
{
public:
    using Ptr = std::shared_ptr<workmodel>;
    workmodel(const chw::EventLoop::Ptr& poller) : _poller(poller)
    {

    }
    virtual ~workmodel() = default;

    virtual void startmodel() = 0;
    virtual void prepare_exit() = 0;

protected:
    chw::EventLoop::Ptr _poller;
};

//命令行参数
struct ConfigCmd
{
    char      role;              // 'c' lient(-c) or 's' erver(-s)
    char     *server_hostname;   // 服务端ip(-c)
    SockNum::SockType protol;    // 协议类型，默认tcp(-u udp)
    int32_t   domain;            // AF_INET(-4) or AF_INET6(-6)
    uint16_t  server_port;       // 服务端的端口号(-p)
    uint32_t  duration;          // 测试总时长，默认0一直测试(-t)
    double    reporter_interval; // 状态输出间隔，默认1，单位秒(-i)
    char     *bind_address;      // 监听的ip地址，默认监听所有ip(-B)
    uint32_t  blksize;           // 发送的每个报文大小(-l)
    uint32_t  bandwidth;         // 设置带宽，默认0以最大能力发送(-b)
    WorkModel workmodel;         // 工作模式，默认 TEXT_MODEL

    char* src;//文件传输模式，本地文件路经(-S)
    char* dst;//文件传输模式，目的文件路经(-D)

    char* save;//日志要保存的文件名(-f)，没有该选项则输出到屏幕

    ConfigCmd()
    {
        role = ' ';
        protol = SockNum::Sock_TCP;
        domain = AF_INET;
        server_port = 0;
        bind_address = nullptr;
        duration = 0;
        reporter_interval = 1;
        bandwidth = 0;
        blksize = 1000;
        server_hostname = nullptr;
        save = nullptr;
        workmodel = TEXT_MODEL;
        src = nullptr;
        dst = nullptr;
    }
};

} // namespace chw
#endif //__COMMON_PROTOCOL_H
