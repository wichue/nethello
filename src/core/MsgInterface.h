// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __MSG_INTERFACE_H
#define __MSG_INTERFACE_H

#include <stdint.h>

namespace chw {

typedef enum _MSG_TYPE_
{
    EM_MSG_BEGIN = 0,

    FILE_TRAN_REQ        = 100,//文件传输请求,C->S
    FILE_TRAN_RSP        = 101,//文件传输响应,S->C
    FILE_TRAN_DATA       = 102,//文件传输数据,C->S
    FILE_TRAN_END        = 103,//文件传输结束,C<->S

    EM_MSG_END
} MsgType;

#pragma pack(push, 1)

// 消息头
typedef struct _MsgHdr_{
    union 
    {
        uint32_t uMsgIndex;// 包序号
        uint32_t uMsgType; // 消息类型
    };
    uint32_t uTotalLen;//包总长度
} MsgHdr;

// 文件传输请求
typedef struct _FileTranReq_ {
    MsgHdr msgHdr;

    char filepath[256];// 文件保存完整路经,包含文件名
    uint32_t filesize; // 文件大小
}FileTranReq;

// 文件传输信令
typedef struct _FileTranSig_ {
    MsgHdr msgHdr;

    uint32_t code;// 错误码
}FileTranSig;

// 传输文件数据
typedef struct _FileTranData_ {
    MsgHdr msgHdr;

    uint8_t pData[0];
}FileTranData;

#pragma pack(pop)

}


#endif//__MSG_INTERFACE_H