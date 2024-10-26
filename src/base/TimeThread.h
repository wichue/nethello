// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __TIME_THREAD_H
#define __TIME_THREAD_H

#include "util.h"

namespace chw {

/**
 * 获取时间差, 返回值单位为秒
 * Get time difference, return value in seconds
 
 * [AUTO-TRANSLATED:43d2403a]
 */
long getGMTOff();

/**
 * 获取1970年至今的毫秒数
 * @param system_time 是否为系统时间(系统时间可以回退),否则为程序启动时间(不可回退)
 */
uint64_t getCurrentMillisecond(bool system_time = false);

/**
 * 获取1970年至今的微秒数
 * @param system_time 是否为系统时间(系统时间可以回退),否则为程序启动时间(不可回退)
 */
uint64_t getCurrentMicrosecond(bool system_time = false);


}//namespace chw

#endif//__TIME_THREAD_H