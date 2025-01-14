// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "FileRecv.h"
#include "ComProtocol.h"
#include "MsgInterface.h"
#include "ErrorCode.h"
#include "File.h"
#include "StickyPacket.h"
#include "uv_errno.h"

namespace chw {

FileRecv::FileRecv()
{
    _status = TRANS_INIT;
    _write_size = 0;
    _filesize = 0;
    _filepath.clear();
    _filename.clear();
}

FileRecv::~FileRecv()
{

}

void FileRecv::SetSndData(std::function<uint32_t(char* buf, uint32_t )> cb)
{
    if(cb != nullptr) {
        _send_data = std::move(cb);
    } else {
        _send_data = [](char* buf, uint32_t len)->uint32_t{
            PrintE("FileRecv not set _send_data, data ignored: %d",len);
            return 0;
        };
    }
}

/**
 * @brief 接收数据回调（epoll线程执行）
 * 
 * @param buf [in]数据
 */
void FileRecv::onRecv(const Buffer::Ptr &buf)
{
    if(StickyPacket(buf,STD_BIND_2(FileRecv::DispatchMsg,this)) == chw::fail)
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
void FileRecv::DispatchMsg(char* buf, uint32_t len)
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
void FileRecv::SendSignalMsg(uint32_t msgtype, uint32_t code)
{
    FileTranSig* psig = (FileTranSig*)_RAM_NEW_(sizeof(FileTranSig));
    psig->msgHdr.uMsgType = msgtype;
    psig->msgHdr.uTotalLen = sizeof(FileTranSig);

    psig->code = code;
        
    if(_send_data((char*)psig,sizeof(FileTranSig)) == 0)
    {
        PrintE("FileRecv: Send Signal Msg failed.");
        sleep_exit(100 * 1000);
    }
    _RAM_DEL_(psig);
}

/**
 * @brief 接收文件传输请求消息
 * 
 * @param buf [in]数据
 */
void FileRecv::procTranReq(char* buf)
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
    InfoL << "recv file, save to:" << pReq->filepath << ",size:" << pReq->filesize << "(bytes)";

    SendSignalMsg(FILE_TRAN_RSP,ERROR_SUCCESS);
    _status = TRANS_SENDING;
}

/**
 * @brief 接收传输结束消息
 * 
 * @param buf [in]数据
 */
void FileRecv::procTranEnd(char* buf)
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
void FileRecv::procFileData(char* buf, uint32_t len)
{
    static bool firstdata = false;
    if(firstdata == false)
    {
        firstdata = true;
        _ticker.resetTime();
    }

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
        ErrorL << "file write failed, error=" << errno << "(" << strerror(errno) << ")"
               << ",write_len=" << write_len << ",len=" << len - sizeof(MsgHdr);
        sleep_exit(100 * 1000);
    }
    
    _write_size += write_len;
    if(_write_size == _filesize)
    {
        fclose(_write_file);
        _write_file = nullptr;

        SendSignalMsg(FILE_TRAN_END,ERROR_SUCCESS);

        auto elapsed = _ticker.elapsedTime();
        double times = (double)elapsed / 1000;//耗时，单位秒

        double speed = 0;// 速率
        std::string unit = "";// 单位
        assert(times);
        speed_human(_filesize / times, speed, unit);
        PrintD("file receive complete! used times:%.2f(sec),speed:%.2f(%s)",times,speed,unit.c_str());
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
        ErrorL << "error: _write_size(" << _write_size << ") > _filesize(" << _filesize << ")";
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
void FileRecv::onError(const SockException &err)
{

}

}//namespace chw