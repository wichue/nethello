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
    if(buf->Size() < head_len)
    {
        return chw::success;
    }

    MsgHdr* pMsgHdr = (MsgHdr*)buf->data();
    //2.如果接收长度小于消息总长度，且消息总长度大于buf缓存长度，则执行扩容
    if(buf->Size() < pMsgHdr->uTotalLen)
    {
        if(pMsgHdr->uTotalLen > buf->Capacity())
        {
            if(pMsgHdr->uTotalLen > MAX_PKT_SIZE)
            {
                PrintE("too big net pkt size:%lu",pMsgHdr->uTotalLen);
                return chw::fail;
            }
            buf->SetCapacity(pMsgHdr->uTotalLen );
        }
    }
    //3.接收长度大于等于消息总长度，分发消息
    else
    {
        uint32_t offset = 0;
        while(true)
        {
            //3.1剩余长度小于消息头长度，结束分发
            if(buf->Size() - offset < head_len)
            {
                break;
            }

            //3.2分发消息
            MsgHdr* pSubHdr = (MsgHdr*)((char*)buf->data() + offset);
            if(buf->Size() - offset >= pSubHdr->uTotalLen)
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
        if(offset < buf->Size())
        {
            return buf->Align(offset, buf->Size());
        }
        else if(offset == buf->Size())
        {
            buf->Reset();
        }
        else
        {
            PrintE("error:offset(%lu) > buf->Size(%s)",offset,buf->Size());
            return chw::fail;
        }
    }
    
    return chw::success;
}

}