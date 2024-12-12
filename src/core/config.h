// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __CONFIG_H
#define __CONFIG_H

// 日志打印不使用颜色，1不使用颜色，0使用颜色
#ifndef LOG_NON_COLOR
#define LOG_NON_COLOR 1
#endif

// 日志打印是否包含前缀（日期、时间、文件名、函数名等），1打印前缀，0不打印前缀
#ifndef LOG_PREFIX
#define LOG_PREFIX 0
#endif

// 是否过滤频繁重复日志，1过滤重复日志，0不过滤重复日志
#ifndef LOG_FILter_REPEAT
#define LOG_FILter_REPEAT 0
#endif

// 发布版本，屏蔽一些影响观瞻的日志，1屏蔽日志，0不屏蔽日志
#ifndef RELEASE_MODEL
#define RELEASE_MODEL 0
#endif

// win定义MAC地址长度
#ifdef _WIN32
#ifndef IFHWADDRLEN
#define IFHWADDRLEN 6
#endif //IFHWADDRLEN
#endif //_WIN32

// windows是否使用npcap库（windows版原始套结字测试），0不使用，1使用
#ifdef _WIN32
#ifndef WIN_USE_NPCAP_LIB
#define WIN_USE_NPCAP_LIB 1
#endif //WIN_USE_NPCAP_LIB
#endif //_WIN32

// npcap pcap_send_queue 队列大小
#ifdef _WIN32
#ifndef NPCAP_SEND_QUEUE_SIZE
#define NPCAP_SEND_QUEUE_SIZE 20
#endif //NPCAP_SEND_QUEUE_SIZE
#endif //_WIN32

// 使用raw、npcap传输文件时，用户数据相对以太头的偏移
#define RAW_FILE_ETH_OFFSET     0

// 原始套结字mtu大小
#define CHW_RAW_MTU 1500

// 文件传输模式是否打印实时速率，0不打印，1打印
#define FILE_MODEL_PRINT_SPEED  1

// 原始套接字发送缓存
#define RAW_SEND_BUFFER 200*1024*1024

// 原始套接字接收缓存
#define RAW_RECV_BUFFER 200*1024*1024

// 文件传输时每包大小
#define FILE_SEND_MTU   1460

#endif//__CONFIG_H