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


#endif//__CONFIG_H