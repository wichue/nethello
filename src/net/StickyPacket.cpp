// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "StickyPacket.h"
#include "MsgInterface.h"

namespace chw {

#define MAX_PKT_SIZE        (256<<20)   //网络包最大长度

/**
 * @brief tcp粘包处理
 * 
 * @param buf   [in]接收数据
 * @param cb    [in]消息分发回调
 * @return uint32_t 成功返回chw::success，发生错误返回chw::fail
 */
uint32_t StickyPacket(const Buffer::Ptr &buf,const DispatchCB& cb)
{
    uint32_t head_len = sizeof(MsgHdr);
    //1. 接收长度小于消息头长度，不处理
    if(buf->RcvLen() < head_len)
    {
        return chw::success;
    }

    MsgHdr* pMsgHdr = (MsgHdr*)buf->data();
    //2.如果接收长度小于消息总长度，且消息总长度大于buf缓存长度，则执行扩容
    if(buf->RcvLen() < pMsgHdr->uTotalLen)
    {
        if(pMsgHdr->uTotalLen > buf->Size())
        {
            if(pMsgHdr->uTotalLen > MAX_PKT_SIZE)
            {
                PrintE("too big net pkt size:%u",pMsgHdr->uTotalLen);
                return chw::fail;
            }
            buf->setCapacity(pMsgHdr->uTotalLen );
        }
    }
    //3.接收长度大于等于消息总长度，分发消息
    else
    {
        uint32_t offset = 0;
        while(true)
        {
            //3.1剩余长度小于消息头长度，结束分发
            if(buf->RcvLen() - offset < head_len)
            {
                break;
            }

            //3.2分发消息
            MsgHdr* pSubHdr = (MsgHdr*)((char*)buf->data() + offset);
            if(buf->RcvLen() - offset >= pSubHdr->uTotalLen)
            {
                cb((char*)buf->data() + offset,pSubHdr->uTotalLen);
                offset += pSubHdr->uTotalLen;
            }
            else
            {
                break;
            }
        }

        //3.3处理结束，如果尾部还有不完整的消息，拷贝到buf的最前端
        if(offset < buf->RcvLen())
        {
            return buf->Align(buf->RcvLen() - offset, buf->RcvLen());
        }
        else if(offset == buf->RcvLen())
        {
            buf->Reset();
        }
        else
        {
            PrintE("error:offset(%u) > buf->RcvLen(%s)",offset,buf->RcvLen());
            return chw::fail;
        }
    }
    
    return chw::success;
}

}