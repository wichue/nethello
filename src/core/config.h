// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __CONFIG_H
#define __CONFIG_H

// 日志打印不使用颜色，1不使用颜色，0使用颜色
#ifndef LOG_NON_COLOR
#define LOG_NON_COLOR 1
#endif

// 日志打印是否包含前缀（日期、时间、文件名、函数名等），1打印前缀，0不打印前缀
#ifndef LOG_PREFIX
#define LOG_PREFIX 0
#endif

// 是否过滤频繁重复日志，1过滤重复日志，0不过滤重复日志
#ifndef LOG_FILter_REPEAT
#define LOG_FILter_REPEAT 0
#endif

// 发布版本，屏蔽一些影响观瞻的日志，1屏蔽日志，0不屏蔽日志
#ifndef RELEASE_MODEL
#define RELEASE_MODEL 0
#endif

// win定义MAC地址长度
#ifdef _WIN32
#ifndef IFHWADDRLEN
#define IFHWADDRLEN 6
#endif //IFHWADDRLEN
#endif //_WIN32

#endif//__CONFIG_H