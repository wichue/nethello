// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include <setjmp.h>// for setjmp
#include <stdio.h>// for printf
#include <stdlib.h>// for exit
#include <signal.h>//for SIG_DFL
#include <memory>

#include "CmdLineParse.h"
#include "SignalCatch.h"
#include "BackTrace.h"
#include "Logger.h"
#include "GlobalValue.h"
#include "Semaphore.h"
#include "TextModel.h"
#include "PressModel.h"
#include "FileModel.h"
#include "util.h"
#if defined(__linux__) || defined(__linux)
#include "RawTextModel.h"
#include "RawPressModel.h"
#include "RawFileModel.h"
#else
#include "NpcapTextModel.h"
#include "NpcapPressModel.h"
#endif// defined(__linux__) || defined(__linux)

chw::workmodel::Ptr _workmodel = nullptr;
using namespace chw;
//捕获ctrl+c
void sigend_handler_abort(int sig)
{
    if(_workmodel)
    {
        _workmodel->prepare_exit();
        usleep(100 * 1000);
        printf("catch abort1 signal:%d,exit now.\n",sig);
        exit(0);
    }
    else
    {
        printf("catch abort2 signal:%d,exit now.\n",sig);
        _exit(0);// 立即强退
    }
}

//捕获段错误
void sigend_handler_crash(int sig)
{
    if(_workmodel)
    {
        _workmodel->prepare_exit();
        printf("catch crash1 signal:%d,exit now.\n",sig);
        usleep(100 * 1000);
        chw::chw_assert();
    }
    else
    {
        printf("catch crash2 signal:%d,exit now.\n",sig);
        usleep(100 * 1000);
        chw::chw_assert();
    }
}

/**
 * 
 * 模式1：文本聊天，客户端连接后，命令行>可以发文本给服务端，服务端命令行可以输入回复文本，支持tcp/udp。
 * 模式2：性能测试，测试tcp速率，udp速率和udp丢包率，可设置带宽、包大小等。
 * 模式3：文件传输，功能类似scp的传文件，先启动服务端后启动客户端，由客户端向服务端发文件，优先实现tcp，udp可结合kcp实现。
 * 模式4：并发测试
 * 模式5：原始套接字测试 SOCK_RAW ，实现文本聊天和性能测试
 * 
 * @brief 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char **argv)
{
    chw::SignalCatch::Instance().CustomAbort(sigend_handler_abort);
    chw::SignalCatch::Instance().CustomCrash(sigend_handler_crash);
    if(chw::CmdLineParse::Instance().parse_arguments(argc,argv) == chw::fail) {
        exit(1);
    }

    // 设置日志系统
    if(chw::gConfigCmd.save == nullptr)
    {
        chw::Logger::Instance().add(std::make_shared<chw::ConsoleChannel>());
    }
    else
    {
        std::shared_ptr<chw::FileChannelBase> fc = std::make_shared<chw::FileChannelBase>();
        if(!fc->setPath(chw::gConfigCmd.save))
        {
            printf("log file path invalid,path=%s\n",chw::gConfigCmd.save);
            exit(1);
        }
        chw::Logger::Instance().add(fc);
    }
    chw::Logger::Instance().setWriter(std::make_shared<chw::AsyncLogWriter>());
    
    switch (chw::gConfigCmd.workmodel)
    {
#if defined(__linux__) || defined(__linux)
    case chw::TEXT_MODEL:
        if (chw::gConfigCmd.protol == SockNum::Sock_RAW) {
            _workmodel = std::make_shared<chw::RawTextModel>();
        }
        else {
            _workmodel = std::make_shared<chw::TextModel>();
        }
        break;
    case chw::PRESS_MODEL:
        if (chw::gConfigCmd.protol == SockNum::Sock_RAW) {
            _workmodel = std::make_shared<chw::RawPressModel>();
        }
        else {
            _workmodel = std::make_shared<chw::PressModel>();
        }
        break;
#else
    case chw::TEXT_MODEL:
        if (chw::gConfigCmd.protol == SockNum::Sock_RAW) {
            _workmodel = std::make_shared<chw::NpcapTextModel>();
        }
        else {
            _workmodel = std::make_shared<chw::TextModel>();
        }
        break;
    case chw::PRESS_MODEL:
        if (chw::gConfigCmd.protol == SockNum::Sock_RAW) {
            _workmodel = std::make_shared<chw::NpcapPressModel>();
        }
        else {
            _workmodel = std::make_shared<chw::PressModel>();
        }
        break;
#endif// defined(__linux__) || defined(__linux)

    case chw::FILE_MODEL:
        if (chw::gConfigCmd.protol == SockNum::Sock_RAW) {
#if defined(__linux__) || defined(__linux)
            _workmodel = std::make_shared<chw::RawFileModel>();
#endif// defined(__linux__) || defined(__linux)
        } else {
            _workmodel = std::make_shared<chw::FileModel>();
        }
        
        break;
    
    default:
        PrintE("unknown work model:%d",chw::gConfigCmd.workmodel);
        exit(1);
    }
    _workmodel->startmodel();


    static chw::Semaphore sem;
    signal(SIGINT, [](int) { sigend_handler_abort(SIGINT); sem.post(); });///SIGINT:Ctrl+C发送信号，结束程序
    sem.wait();
    usleep(100 * 1000);

    chw::SignalCatch::Instance().CustomAbort(SIG_DFL);
    chw::SignalCatch::Instance().CustomCrash(SIG_DFL);


    exit(0); 
}
