// Copyright (c) 2024 The idump project authors. SPDX-License-Identifier: MIT.
// This file is part of idump(https://github.com/wichue/idump).

#ifndef        __BACK_TRACE_H
#define        __BACK_TRACE_H

namespace chw {

/**
 * @brief 自定义中断，linux下输出调用栈
 * 
 */
void chw_assert();
#ifndef chw_assert
#define chw_assert() chw_assert()
#endif

}// namespace chw
#endif // __BACK_TRACE_H
