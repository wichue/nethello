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

chw::workmodel::Ptr _workmodel = nullptr;
using namespace chw;
//捕获ctrl+c
void sigend_handler_abort(int sig)
{
    if(_workmodel)
    {
        _workmodel->prepare_exit();
    }
    usleep(100 * 1000);
    
    printf("catch abort signal:%d,exit now.\n",sig);
    exit(0);
}

//捕获段错误
void sigend_handler_crash(int sig)
{
    if(_workmodel)
    {
        _workmodel->prepare_exit();
    }
    usleep(100 * 1000);
    
    printf("catch crash signal:%d,exit now.\n",sig);
	chw::chw_assert();
}

//todo:windows
/**
 * 
 * 模式1：文本聊天，客户端连接后，命令行>可以发文本给服务端，服务端命令行可以输入回复文本，支持tcp/udp。
 * 模式2：压力测试，测试tcp速率，udp速率和udp丢包率，可设置带宽、包大小等。
 * 模式3：文件传输，功能类似scp的传文件，先启动服务端后启动客户端，由客户端向服务端发文件，优先实现tcp，udp可结合kcp实现。
 * 模式4：并发测试
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
    case chw::TEXT_MODEL:
        _workmodel = std::make_shared<chw::TextModel>();
        break;
    case chw::PRESS_MODEL:
        _workmodel = std::make_shared<chw::PressModel>();
        break;
    case chw::FILE_MODEL:
        _workmodel = std::make_shared<chw::FileModel>();
        break;
    
    default:
        PrintE("unknown work model:%d",chw::gConfigCmd.workmodel);
        exit(1);
    }
    _workmodel->startmodel();


    static chw::Semaphore sem;
    signal(SIGINT, [](int) { sigend_handler_abort(SIGINT); sem.post(); });///SIGINT:Ctrl+C发送信号，结束程序
    sem.wait();

    chw::SignalCatch::Instance().CustomAbort(SIG_DFL);
    chw::SignalCatch::Instance().CustomCrash(SIG_DFL);


    exit(0); 
}
