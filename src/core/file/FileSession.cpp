#include "FileSession.h"
#include "ComProtocol.h"
#include "MsgInterface.h"
#include "ErrorCode.h"

namespace chw {

FileSession::FileSession(const Socket::Ptr &sock) :Session(sock)
{
    _cls = chw::demangle(typeid(FileSession).name());
    _status = TRANS_INIT;
}

FileSession::~FileSession()
{

}

void FileSession::onRecv(const Buffer::Ptr &buf)
{
    MsgHdr* pMsgHdr = (MsgHdr*)buf->data();
    switch(pMsgHdr->uMsgType)
    {
        case FILE_TRAN_REQ:
            procTranReq(buf);
            break;
        case FILE_TRAN_END:
            procTranEnd(buf);
        default:
            break;
    }
}

void FileSession::procTranReq(const Buffer::Ptr &pBuf)
{
    FileTranReq* pReq = (FileTranReq*)pBuf->data();
    _filesize = pReq->filesize;
    _RAM_CPY_(_filepath,256,pReq->filepath,256);

    if(_filesize == 0)
    {    
        // 通知对方发送结果
        FileTranSig* psig = (FileTranSig*)_RAM_NEW_(sizeof(FileTranSig));
        psig->msgHdr.uMsgType = FILE_TRAN_RSP;
        psig->msgHdr.uTotalLen = sizeof(FileTranSig);

        psig->code = ERROR_FILE_INVALID_SIZE;
        
        senddata((uint8_t*)psig,sizeof(FileTranSig));
        _RAM_DEL_(psig);
        sleep_exit(100 * 1000);
    }
}

void FileSession::procTranEnd(const Buffer::Ptr &pBuf)
{
    FileTranSig* pSig = (FileTranSig*)pBuf->data();
    if(pSig->code != ERROR_SUCCESS)
    {
        PrintE("peer error:%s",Error2Str(pSig->code).c_str());
        sleep_exit(100 * 1000);
    }
}

void FileSession::onError(const SockException &err)
{

}

void FileSession::onManager()
{

}

const std::string &FileSession::className() const 
{
    return _cls;
}

/**
 * @brief 发送数据
 * 
 * @param buff 数据
 * @param len  数据长度
 * @return uint32_t 发送成功的数据长度
 */
uint32_t FileSession::senddata(uint8_t* buff, uint32_t len)
{
    if(getSock()) {
        return getSock()->send_i(buff,len);
    } else {
        PrintE("tcp session already disconnect");
        return 0;
    }
}

}//namespace chw