// Copyright (c) 2024 The idump project authors. SPDX-License-Identifier: MIT.
// This file is part of idump(https://github.com/wichue/idump).

#include "SignalCatch.h"
#include <signal.h>

namespace chw {

INSTANCE_IMP(SignalCatch)

SignalCatch::SignalCatch()
{

}

SignalCatch::~SignalCatch()
{

}

/**
 * @brief catch signal，处理用户操作导致的中断
 * 
 */
void SignalCatch::CustomAbort(void (*handler)(int))
{
#ifdef SIGINT// 由应用程序用户生成的交互式注意信号，Ctrl+c
    signal(SIGINT, handler);
#endif
#ifdef SIGTERM// 请求中止进程，kill命令缺省发送
    signal(SIGTERM, handler);
#endif
#ifdef SIGHUP// 当terminal 被disconnect时候发送
    signal(SIGHUP, handler);
#endif
}

/**
 * @brief catch signal，处理非用户操作导致的中断 
 * 
 * @param handler 
 */
void SignalCatch::CustomCrash(void (*handler)(int))
{
#ifdef SIGSEGV//  segmentation violation，无效的内存引用，或发生段错误
    signal(SIGSEGV, handler);
#endif

#ifdef SIGILL// illegal instruction，执行非法指令，CPU架构不匹配，.so文件破坏，代码段破坏等
    signal(SIGILL, handler);
#endif

#ifdef SIGBUS// bus error，总线错误
    signal(SIGBUS, handler);
#endif

#ifdef SIGFPE// floating-point exception，算术运算异常，整数除以0，/整数浮点数溢出等
    signal(SIGFPE, handler);
#endif

#ifdef SIGABRT//abort program，异常退出；通常是调用 abort(), raise(), kill(), pthread_kill() 或者被系统进程杀死
    signal(SIGABRT, handler);
#endif

#ifdef SIGSTKFLT// stack fault，处理器栈故障
    signal(SIGSTKFLT, handler);
#endif

#ifdef SIGSYS// 无效的 linux 内核系统调用
    signal(SIGSYS, handler);
#endif

#ifdef SIGTRAP// gdb 调试设置断点等操作使用的信号
    // signal(SIGTRAP, handler);
#endif
}


}//namespace chw
