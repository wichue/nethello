// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef        __READ_LINE_H
#define        __READ_LINE_H

#include <stdint.h>
#include <string>
#include <iostream>

namespace chw {

/**
 * @brief 输出数据到命令行终端
 * 
 * @param prompt 数据
 */
void stdout2line(const char* prompt)
{
    if(prompt)
    {
        fputs(prompt, stdout);
        fflush(stdout);
    }
}

/**
 * @brief 读取命令行输入
 * 
 * @param prompt 要输出到命令行的内容
 * @param msg    读取的数据
 */
void ReadLine(std::string& msg, const char* prompt = nullptr)
{
    stdout2line(prompt);
    std::getline(std::cin, msg);
}

}//namespace chw

#endif//__READ_LINE_H