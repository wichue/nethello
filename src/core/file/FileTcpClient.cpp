#include "FileTcpClient.h"
#include <sys/stat.h>
#include "MsgInterface.h"
#include "ErrorCode.h"
#include "GlobalValue.h"

namespace chw {

FileTcpClient::FileTcpClient(const EventLoop::Ptr &poller) : TcpClient(poller)
{
    _status = TRANS_INIT;
    _send_poller = nullptr;
}

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
    _RAM_CPY_(preq->filepath,256,gConfigCmd.dst,strlen(gConfigCmd.dst));
    preq->filesize = filesize;

    senddata((uint8_t*)preq,sizeof(FileTranReq));
    _RAM_DEL_(preq);
}

// 接收数据回调（epoll线程执行）
//todo:需要解决粘包后再分发处理
void FileTcpClient::onRecv(const Buffer::Ptr &pBuf)
{
    MsgHdr* pMsgHdr = (MsgHdr*)pBuf->data();
    switch(pMsgHdr->uMsgType)
    {
        case FILE_TRAN_RSP:
            procTranRsp(pBuf);
            break;
        case FILE_TRAN_END:
            procTranEnd(pBuf);
            break;

        default:
            break;
    }

    pBuf->Reset();
}

void FileTcpClient::procTranEnd(const Buffer::Ptr &pBuf)
{
    FileTranSig* pSig = (FileTranSig*)pBuf->data();
    if(pSig->code != ERROR_SUCCESS)
    {
        PrintE("peer error:%s",Error2Str(pSig->code).c_str());
        sleep_exit(100 * 1000);
    }
}

void FileTcpClient::procTranRsp(const Buffer::Ptr &pBuf)
{
    FileTranSig* pSig = (FileTranSig*)pBuf->data();
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
            uint32_t len = strong_self->senddata((uint8_t*)buf,uReadSize);
            if(len == 0)
            {
                break;
            }
            else
            {
                allSendLen += len;
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

void FileTcpClient::localTransEnd(uint32_t status)
{
    _status = status;
    uint32_t code = 0;
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