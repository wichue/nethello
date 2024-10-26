// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef        __CMD_LINE_PARSE_H
#define        __CMD_LINE_PARSE_H

#include "util.h"

namespace chw {

class CmdLineParse {
public:
    CmdLineParse() = default;
    ~CmdLineParse() = default;

    static CmdLineParse &Instance();

    /**
     * @brief 解析命令行参数
     * 
     * @param argc		[in]参数数量
     * @param argv		[in]参数指针
     * @return uint32_t 成功返回chw::success,失败返回chw::fail
     */
    uint32_t parse_arguments(int argc, char **argv);
private:
    /**
     * @brief 打印帮助
     */
	void help();

    /**
     * @brief 打印版本
     */
	void version();
};

}//namespace chw
#endif //__CMD_LINE_PARSE_H
