#include "FileTcpClient.h"
#include <sys/stat.h>
#include "MsgInterface.h"
#include "ErrorCode.h"
#include "GlobalValue.h"
#include "File.h"
#include "StickyPacket.h"

namespace chw {

FileTcpClient::FileTcpClient(const EventLoop::Ptr &poller) : TcpClient(poller)
{
    _status = TRANS_INIT;
    _send_poller = nullptr;
}

/**
 * @brief 开始文件传输
 * 
 */
void FileTcpClient::StartTransf()
{
    _status = TRANS_CONNECTED;
    struct stat st;
    if (stat(gConfigCmd.src, &st))
    {
        PrintE("stat file %s failed, errno=%d errmsg=%s", gConfigCmd.src, errno, strerror(errno));
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
    PrintD("preq->filepath=%s,filesize=%u",preq->filepath,preq->filesize);

    senddata((uint8_t*)preq,sizeof(FileTranReq));
    _RAM_DEL_(preq);
}

/**
 * @brief 分发消息
 * 
 * @param buf [in]消息
 * @param len [in]长度
 */
void FileTcpClient::DispatchMsg(char* buf, uint32_t len)
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
void FileTcpClient::onRecv(const Buffer::Ptr &pBuf)
{
    if(StickyPacket(pBuf,STD_BIND_2(FileTcpClient::DispatchMsg,this)) == chw::fail)
    {
        sleep_exit(100 * 1000);
    }
}

/**
 * @brief 收到传输结束消息
 * 
 * @param pBuf [in]消息
 */
void FileTcpClient::procTranEnd(char* buf)
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
void FileTcpClient::procTranRsp(char* buf)
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
    std::weak_ptr<FileTcpClient> weak_self = std::static_pointer_cast<FileTcpClient>(shared_from_this());
    _send_poller->async([weak_self,poller](){
        struct stat st;
        if (stat(gConfigCmd.src, &st))
        {
            PrintE("stat file %s failed, errno=%d errmsg=%s", gConfigCmd.src, errno, strerror(errno));
            sleep_exit(100 * 1000);
        }

        uint32_t filesize = st.st_size;

        if (!filesize)
        {
            PrintE("file is empty:%s",gConfigCmd.src);
            sleep_exit(100 * 1000);
        }

        char* buf;	
	    try {
	        buf = (char*)_RAM_NEW_(TCP_BUFFER_SIZE);
	    } catch(std::bad_alloc) {
	    	PrintE("malloc buf failed, size=%u",TCP_BUFFER_SIZE);
	    	sleep_exit(100 * 1000);
	    }

        FILE* fp = fopen(gConfigCmd.src, "rb");
        if (!fp)
        {
            PrintE("open file %s failed, errno=%d errmsg=%s", gConfigCmd.src, errno, strerror(errno));
            sleep_exit(100 * 1000);
        }

        auto strong_self = weak_self.lock();
        if (!strong_self) {
            _RAM_DEL_(buf);
            return;
        }
        MsgHdr* pMsgHdr = (MsgHdr*)buf;
        pMsgHdr->uMsgType = FILE_TRAN_DATA;

        uint32_t uReadSize = 0;
        uint32_t allSendLen = 0;
        while ((uReadSize = fread(buf + sizeof(MsgHdr), 1, TCP_BUFFER_SIZE - sizeof(MsgHdr), fp)) > 0)
        {
            pMsgHdr->uTotalLen = uReadSize + sizeof(MsgHdr);
            uint32_t len = strong_self->senddata((uint8_t*)buf,uReadSize + sizeof(MsgHdr));
            if(len == 0)
            {
                break;
            }
            else
            {
                allSendLen += uReadSize;
            }
        }
        fclose(fp);
        _RAM_DEL_(buf);

        uint32_t status = 0;
        if(allSendLen == filesize)
        {
            //发送成功
            status = TRANS_SEND_OVER;
        }
        else
        {
            //发送失败
            PrintE("send failed, filesize=%u, allSendLen=%u",filesize,allSendLen);
            status = TRANS_SEND_FAIL;
        }
        
        // 切回原来的线程
        poller->async([status,weak_self](){
            auto strong_self = weak_self.lock();
            if (!strong_self) {
                return;
            }

            strong_self->localTransEnd(status);
        });
    });
}

/**
 * @brief 本端文件发送结束
 * 
 * @param status [in]状态码(FileTranStatus)
 */
void FileTcpClient::localTransEnd(uint32_t status)
{
    _status = status;
    uint32_t code = ERROR_SUCCESS;
    if(status == TRANS_SEND_OVER)
    {
        PrintD("file send complete!");
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
        
    senddata((uint8_t*)psig,sizeof(FileTranSig));
    _RAM_DEL_(psig);
    sleep_exit(100 * 1000);
}

// 错误回调
void FileTcpClient::onError(const SockException &ex)
{

}

}//namespace chw