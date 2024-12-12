// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "FileSend.h"
#include <sys/stat.h>
#include "MsgInterface.h"
#include "ErrorCode.h"
#include "GlobalValue.h"
#include "File.h"
#include "StickyPacket.h"
#include "BackTrace.h"
#include "uv_errno.h"
#include "config.h"

namespace chw {

FileSend::FileSend(const EventLoop::Ptr &poller) : FileTransfer()
{
    _status = TRANS_INIT;
    _send_poller = nullptr;
    _poller = poller;
    _snd_buf_mtu = FILE_SEND_MTU;
}

void FileSend::SetSndData(std::function<uint32_t(char* buf, uint32_t )> cb)
{
    if(cb != nullptr) {
        _send_data = std::move(cb);
    } else {
        _send_data = [](char* buf, uint32_t len)->uint32_t{
            PrintE("FileSend not set _send_data, data ignored: %d",len);
            return 0;
        };
    }
}

/**
 * @brief 开始文件传输
 * 
 */
void FileSend::StartTransf()
{
    _status = TRANS_CONNECTED;
    struct stat st;
    if (stat(gConfigCmd.src, &st))
    {
        PrintE("stat file %s failed, errmsg=%s", gConfigCmd.src, get_uv_errmsg(true));
        sleep_exit(100 * 1000);
    }

    uint32_t filesize = st.st_size;

    if (filesize == 0)
    {
        PrintE("file is empty:%s",gConfigCmd.src);
        sleep_exit(100 * 1000);
    }

    FileTranReq* preq = (FileTranReq*)_RAM_NEW_(sizeof(FileTranReq));
    preq->msgHdr.uMsgType = FILE_TRAN_REQ;
    preq->msgHdr.uTotalLen = sizeof(FileTranReq);

    // 目的路经
    std::string dst_path = gConfigCmd.dst;
    if(dst_path[dst_path.size() - 1] != '/')
    {
        dst_path.append("/");
    }

    snprintf(preq->filepath,256,"%s%s",dst_path.c_str(),path_Name(gConfigCmd.src).c_str());
    preq->filesize = filesize;
    InfoL << "preq->filepath=" << preq->filepath << ",filesize=" << preq->filesize << "(bytes)";

    if(_send_data((char*)preq,sizeof(FileTranReq)) == 0)
    {
        PrintE("send file tranf req failed.");
        sleep_exit(100 * 1000);
    }
    _RAM_DEL_(preq);
}

void FileSend::SetSndBufMtu(uint32_t mtu)
{
    _snd_buf_mtu = mtu;
}

/**
 * @brief 分发消息
 * 
 * @param buf [in]消息
 * @param len [in]长度
 */
void FileSend::DispatchMsg(char* buf, uint32_t len)
{
    MsgHdr* pMsgHdr = (MsgHdr*)buf;
    switch(pMsgHdr->uMsgType)
    {
        case FILE_TRAN_RSP:
            procTranRsp(buf);
            break;
        case FILE_TRAN_END:
            procTranEnd(buf);
            break;

        default:
            break;
    }
}

// 接收数据回调（epoll线程执行）
void FileSend::onRecv(const Buffer::Ptr &pBuf)
{
    if(StickyPacket(pBuf,STD_BIND_2(FileSend::DispatchMsg,this)) == chw::fail)
    {
        sleep_exit(100 * 1000);
    }
}

/**
 * @brief 收到传输结束消息
 * 
 * @param pBuf [in]消息
 */
void FileSend::procTranEnd(char* buf)
{
    FileTranSig* pSig = (FileTranSig*)buf;
    if(pSig->code != ERROR_SUCCESS)
    {
        PrintE("peer error:%s",Error2Str(pSig->code).c_str());
        sleep_exit(100 * 1000);
    }
}

/**
 * @brief 收到文件发送响应
 * 
 * @param pBuf [in]响应数据
 */
void FileSend::procTranRsp(char* buf)
{
    FileTranSig* pSig = (FileTranSig*)buf;
    if(pSig->code != ERROR_SUCCESS)
    {
        PrintE("peer error:%s",Error2Str(pSig->code).c_str());
        sleep_exit(100 * 1000);
    }

    if(_status == TRANS_CONNECTED)
    {
        _status = TRANS_SENDING;
    }
    else
    {
        return;
    }

    // 创建独立线程发送文件数据，避免阻塞信令
    if(_send_poller == nullptr)
    {
        _send_poller = std::make_shared<EventLoop>("send poller");
    }
    
    auto poller = _poller;
    std::weak_ptr<FileSend> weak_self = std::static_pointer_cast<FileSend>(shared_from_this());
    _send_poller->async([weak_self,poller](){
        struct stat st;
        if (stat(gConfigCmd.src, &st))
        {
            PrintE("stat file %s failed, errmsg=%s", gConfigCmd.src, get_uv_errmsg(true));
            sleep_exit(100 * 1000);
        }

        uint32_t filesize = st.st_size;

        if (!filesize)
        {
            PrintE("file is empty:%s",gConfigCmd.src);
            sleep_exit(100 * 1000);
        }

        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }

        char* buf;	
	    try {
	        buf = (char*)_RAM_NEW_(strong_self->_snd_buf_mtu);
	    } catch(std::bad_alloc) {
            ErrorL << "malloc buf failed, size=" << strong_self->_snd_buf_mtu;
	    	sleep_exit(100 * 1000);
	    }

        FILE* fp = fopen(gConfigCmd.src, "rb");
        if (!fp)
        {
            PrintE("open file %s failed, errmsg=%s", gConfigCmd.src, get_uv_errmsg(true));
            sleep_exit(100 * 1000);
        }

        MsgHdr* pMsgHdr = (MsgHdr*)buf;
        pMsgHdr->uMsgType = FILE_TRAN_DATA;

        // 控速
        double uMBps = (double)gConfigCmd.bandwidth;// 每秒需要发送多少MB数据
        uint32_t uByteps = uMBps * 1024 * 1024;// 每秒需要发送多少byte数据
        uint32_t uBytep100ms = uByteps / 10;// 每100ms需要发送多少byte数据
        uint32_t curr_all_sndlen = 0;// 当前周期发送长度
        static Ticker ticker_ctl;// 控速用的计时器
        ticker_ctl.resetTime();

        Ticker tker;// 统计耗时
        uint32_t uReadSize = 0;
        uint32_t allSendLen = 0;
        while ((uReadSize = fread(buf + sizeof(MsgHdr), 1, strong_self->_snd_buf_mtu - sizeof(MsgHdr), fp)) > 0)
        {
            pMsgHdr->uTotalLen = uReadSize + sizeof(MsgHdr);
            uint32_t len = strong_self->_send_data((char*)buf,uReadSize + sizeof(MsgHdr));
            if(len == 0)
            {
                break;
            }
            else
            {
                allSendLen += uReadSize;
            }

            if(uBytep100ms != 0)
            {
                curr_all_sndlen += uReadSize + sizeof(MsgHdr);

                uint16_t use_ms =  ticker_ctl.elapsedTime();
                if(use_ms >= 100)
                {
                    ticker_ctl.resetTime();
                    curr_all_sndlen = 0;
                }

                if(curr_all_sndlen >= uBytep100ms)
                {
                    ticker_ctl.resetTime();
                    curr_all_sndlen = 0;
                    usleep((100 - use_ms) * 1000);
                }
            }
        }
        fclose(fp);
        _RAM_DEL_(buf);

        auto elapsed = tker.elapsedTime();
        double times = (double)elapsed / (double)1000;//耗时，单位秒
        // 文件小速度快，elapsed可能是0
        if(times <= 0)
        {
            times = 0.001;
        }

        uint32_t status = 0;
        if(allSendLen == filesize)
        {
            //发送成功
            status = TRANS_SEND_OVER;
        }
        else
        {
            //发送失败
            ErrorL << "send failed, filesize=" << filesize << ", allSendLen=" << allSendLen;
            status = TRANS_SEND_FAIL;
        }
        
        // 切回原来的线程
        poller->async([status,weak_self,times,filesize](){
            auto strong_self = weak_self.lock();
            if (!strong_self) {
                return;
            }

            strong_self->localTransEnd(status,filesize,times);
        });
    });
}

/**
 * @brief 本端文件发送结束
 * 
 * @param status [in]状态码(FileTranStatus)
 */
void FileSend::localTransEnd(uint32_t status, uint32_t filesize, double times)
{
    _status = status;
    uint32_t code = ERROR_SUCCESS;
    if(status == TRANS_SEND_OVER)
    {
        double speed = 0;// 速率
        std::string unit = "";// 单位
        assert(times);
        speed_human(filesize / times, speed, unit);
        PrintD("file send complete! used times:%.2f(sec),speed:%.2f(%s)",times,speed,unit.c_str());
    }
    else
    {
        PrintE("file send failed!");
        code = ERROR_FILE_SEND_FAIL;
    }

    // 通知对方发送结果
    FileTranSig* psig = (FileTranSig*)_RAM_NEW_(sizeof(FileTranSig));
    psig->msgHdr.uMsgType = FILE_TRAN_END;
    psig->msgHdr.uTotalLen = sizeof(FileTranSig);

    psig->code = code;
        
    _send_data((char*)psig,sizeof(FileTranSig));
    _RAM_DEL_(psig);
    // sleep_exit(100 * 1000);
}

// 错误回调
void FileSend::onError(const SockException &ex)
{

}

}//namespace chw