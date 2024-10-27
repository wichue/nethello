// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "FileSession.h"
#include "ComProtocol.h"
#include "MsgInterface.h"
#include "ErrorCode.h"
#include "File.h"
#include "StickyPacket.h"

namespace chw {

FileSession::FileSession(const Socket::Ptr &sock) :Session(sock)
{
    _cls = chw::demangle(typeid(FileSession).name());
    
    _status = TRANS_INIT;
    _write_size = 0;
    _filesize = 0;
    _filepath.clear();
    _filename.clear();
}

FileSession::~FileSession()
{

}

/**
 * @brief 接收数据回调（epoll线程执行）
 * 
 * @param buf [in]数据
 */
void FileSession::onRecv(const Buffer::Ptr &buf)
{
    if(StickyPacket(buf,STD_BIND_2(FileSession::DispatchMsg,this)) == chw::fail)
    {
        sleep_exit(100 * 1000);
    }
}

/**
 * @brief 分发消息
 * 
 * @param buf [in]消息
 * @param len [in]长度
 */
void FileSession::DispatchMsg(char* buf, uint32_t len)
{
    MsgHdr* pMsgHdr = (MsgHdr*)buf;
    switch(pMsgHdr->uMsgType)
    {
        case FILE_TRAN_REQ:
            procTranReq(buf);
            break;
        case FILE_TRAN_END:
            procTranEnd(buf);
            break;
        case FILE_TRAN_DATA:
            procFileData(buf,len);
            break;

        default:
            break;
    }
}

/**
 * @brief 发送信令消息给对端
 * 
 * @param msgtype [in]消息类型
 * @param code    [in]错误码
 */
void FileSession::SendSignalMsg(uint32_t msgtype, uint32_t code)
{
    FileTranSig* psig = (FileTranSig*)_RAM_NEW_(sizeof(FileTranSig));
    psig->msgHdr.uMsgType = msgtype;
    psig->msgHdr.uTotalLen = sizeof(FileTranSig);

    psig->code = code;
        
    senddata((char*)psig,sizeof(FileTranSig));
    _RAM_DEL_(psig);
}

/**
 * @brief 接收文件传输请求消息
 * 
 * @param buf [in]数据
 */
void FileSession::procTranReq(char* buf)
{
    if(_status != TRANS_INIT)
    {
        return;
    }
    _status = TRANS_CONNECTED;

    FileTranReq* pReq = (FileTranReq*)buf;
    _filesize = pReq->filesize;
    _filepath = path_Dir(pReq->filepath);
    _filename = path_Name(pReq->filepath);

    if(_filesize == 0)
    {    
        SendSignalMsg(FILE_TRAN_RSP,ERROR_FILE_INVALID_SIZE);
        PrintE("error: invalid file size");
        sleep_exit(100 * 1000);
    }

    if(_filepath.size() == 0)
    {
        SendSignalMsg(FILE_TRAN_RSP,ERROR_FILE_INVALID_FILE_PATH);
        PrintE("error: file transf, invalid file path");
        sleep_exit(100 * 1000);
    }
    if(_filename.size() == 0)
    {
        SendSignalMsg(FILE_TRAN_RSP,ERROR_FILE_INVALID_FILE_NAME);
        PrintE("error: file transf, invalid file name");
        sleep_exit(100 * 1000);
    }

    if(is_dir(_filepath.c_str()) == false)
    {
        uint32_t code = 0;
        // No such file or directory
        if(errno == 2)
        {
            PrintE("error: No such file or directory:%s",_filepath.c_str());
            code = ERROR_FILE_PEER_NO_DIR;
        }
        // Permission denied
        else if(errno == 13)
        {
            PrintE("error: Permission denied:%s",_filepath.c_str());
            code = ERROR_FILE_PEER_PERM_DENIED;
        }
        else
        {
            PrintE("error: %d(%s)",errno,strerror(errno));
            code = ERROR_FILE_PEER_UNKNOWN;
        }

        
        SendSignalMsg(FILE_TRAN_RSP,code);
        sleep_exit(100 * 1000);
    }

    _write_file = fopen(pReq->filepath, "wb");
	if (!_write_file)
	{
		uint32_t code = 0;
        // No such file or directory
        if(errno == 2)
        {
            PrintE("error: No such file or directory:%s",_filepath.c_str());
            code = ERROR_FILE_PEER_NO_DIR;
        }
        // Permission denied
        else if(errno == 13)
        {
            PrintE("error: Permission denied:%s",_filepath.c_str());
            code = ERROR_FILE_PEER_PERM_DENIED;
        }
        else
        {
            PrintE("error: %d(%s)",errno,strerror(errno));
            code = ERROR_FILE_PEER_UNKNOWN;
        }

        SendSignalMsg(FILE_TRAN_RSP,code);
        sleep_exit(100 * 1000);
	}
    PrintD("recv file:%s,size:%u(bytes)",pReq->filepath,pReq->filesize);

    SendSignalMsg(FILE_TRAN_RSP,ERROR_SUCCESS);
    _status = TRANS_SENDING;
}

/**
 * @brief 接收传输结束消息
 * 
 * @param buf [in]数据
 */
void FileSession::procTranEnd(char* buf)
{
    FileTranSig* pSig = (FileTranSig*)buf;
    if(pSig->code != ERROR_SUCCESS)
    {
        if (_write_file)
        {
            fclose(_write_file);
        }
        PrintE("peer error:%s",Error2Str(pSig->code).c_str());
        sleep_exit(100 * 1000);
    }
}

/**
 * @brief 接收文件传输数据
 * 
 * @param buf [in]数据
 * @param len [in]长度
 */
void FileSession::procFileData(char* buf, uint32_t len)
{
    if (!_write_file)
    {
        PrintE("write file is null");
        sleep_exit(100 * 1000);
    }

    if(len <= sizeof(MsgHdr))
    {
        return;
    }

    uint32_t write_len = fwrite(buf + sizeof(MsgHdr), 1, len - sizeof(MsgHdr), _write_file);
    if(write_len != len - sizeof(MsgHdr))
    {
        fclose(_write_file);

        SendSignalMsg(FILE_TRAN_END,ERROR_FILE_PEER_UNKNOWN);
        PrintE("file write failed, error=%d(%s),write_len=%u,len=%u",errno,strerror(errno),write_len,len - sizeof(MsgHdr));
        sleep_exit(100 * 1000);
    }
    
    _write_size += write_len;
    if(_write_size == _filesize)
    {
        fclose(_write_file);
        _write_file = nullptr;

        SendSignalMsg(FILE_TRAN_END,ERROR_SUCCESS);
        PrintD("file receive complete!");
        // sleep_exit(100 * 1000);  

        // 重置条件，继续接收其他文件
        _status = TRANS_INIT;
        _write_size = 0;
        _filepath.clear();
        _filename.clear();
        _filesize = 0;
    }
    else if(_write_size > _filesize)
    {
        fclose(_write_file);

        SendSignalMsg(FILE_TRAN_END,ERROR_FILE_PEER_UNKNOWN);
        PrintE("error: _write_size(%u) > _filesize(%u)",_write_size, _filesize);
        sleep_exit(100 * 1000);
    }
    else
    {
        // 继续接收
    }
}

/**
 * @brief 发生错误时的回调
 * 
 * @param err 异常
 */
void FileSession::onError(const SockException &err)
{

}

/**
 * @brief 定时器周期管理回调
 * 
 */
void FileSession::onManager()
{

}

/**
 * @brief 返回当前类名称
 * 
 * @return const std::string& 类名称
 */
const std::string &FileSession::className() const 
{
    return _cls;
}

/**
 * @brief 发送数据
 * 
 * @param buff [in]数据
 * @param len  [in]数据长度
 * @return uint32_t 发送成功的数据长度
 */
uint32_t FileSession::senddata(char* buff, uint32_t len)
{
    if(getSock()) {
        return getSock()->send_i(buff,len);
    } else {
        PrintE("tcp session already disconnect");
        return 0;
    }
}

}//namespace chw