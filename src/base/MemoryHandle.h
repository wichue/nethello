// Copyright (c) 2024 The idump project authors. SPDX-License-Identifier: MIT.
// This file is part of idump(https://github.com/wichue/idump).

#ifndef        __MEMORY_HANDLE_H
#define        __MEMORY_HANDLE_H

#include <iostream>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

#ifndef _MEMORY_PROTECT_
#define _MEMORY_PROTECT_ 0
#endif //_MEMORY_PROTECT_

#define MAX_ALLOC_MEMORY (500*1024*1024)

#define _RAM_NEW_(len) chw::malloc(len)
#define _RAM_DEL_(ptr) chw::free(ptr)
#define _RAM_SET_(pDst,uDstLen,SetValue,uSetLen) chw::s_memset(pDst,uDstLen,SetValue,uSetLen)
#define _RAM_ZERQ_(pDst,uDstLen,uSetPos) chw::s_memzero(pDst,uDstLen,uSetPos)
#define _RAM_CPY_(pDst,uDstLen,pSrc,uCpyLen) chw::s_memcpy(pDst,uDstLen,pSrc,uCpyLen)
#define _RAM_CMP_(pDst,uDstLen,pSrc,uCmpLen) chw::s_memcmp(pDst,uDstLen,pSrc,uCmpLen)

namespace chw {

/**
* @brief 内存分配
* @param size 分配大小
* @return 分配的内存
*/
static inline void *malloc(size_t size)
{
#if 0
    if(size > MAX_ALLOC_MEMORY)
        assert(0);
#endif
    void *buf = NULL;
    try {
        buf = ::new char[size];
    } catch (...) {
        throw std::bad_alloc();
    }

    return buf;
}

/**
 * @brief 内存释放
 * @param buf 释放的内存
 */
static inline void free(void *buf)
{
    delete[] (char*)buf;
}

/**
 * @brief 内存设置
 * @param pDst      指向要设置的内存的指针
 * @param uDstLen   要设置的内存的大小
 * @param SetValue  要设置的值
 * @param uSetLen   要设置的长度
 * @return          指向pDst的指针
 */
static inline void *s_memset(void *pDst, ssize_t uDstLen, int32_t SetValue, ssize_t uSetLen)
{
#if _MEMORY_PROTECT_
    assert(uDstLen>=uSetLen);
    assert(uDstLen>=0 && uDstLen < MAX_ALLOC_MEMORY
           && uSetLen >=0 && uSetLen<MAX_ALLOC_MEMORY);
#else
    (void)uDstLen;
#endif
    return memset(pDst,SetValue,uSetLen);
}

/**
 * @brief 将内存的某一个字节置为0
 * @param pDst      指向要设置的内存的指针
 * @param uDstLen   要设置的内存的大小
 * @param uSetPos   要置为0的内存的偏移
 */
static inline void s_memzero(void *pDst, ssize_t uDstLen, ssize_t uSetPos)
{
#if _MEMORY_PROTECT_
    assert(uDstLen>=uSetPos+1);
    assert(uDstLen>=0 && uDstLen < MAX_ALLOC_MEMORY
           && uSetPos >=0 && uSetPos<MAX_ALLOC_MEMORY);
#else
    (void)uDstLen;
#endif
    ((uint8_t *)pDst)[uSetPos] = '\0';
}

/**
 * @brief 内存拷贝
 * @param pDst      指向目的内存的指针
 * @param uDstLen   目的内存的大小
 * @param pSrc      指向源内存的指针
 * @param uCpyLen   要拷贝的长度
 */
static inline void s_memcpy(void *pDst,ssize_t uDstLen,const void * const pSrc,ssize_t uCpyLen)
{
#if _MEMORY_PROTECT_
    assert(uDstLen>=uCpyLen);
    assert(uDstLen>=0 && uDstLen < MAX_ALLOC_MEMORY
           && uCpyLen >=0 && uCpyLen<MAX_ALLOC_MEMORY);
#else
    (void)uDstLen;
#endif
    memcpy(pDst,pSrc,uCpyLen);
}

/**
 * @brief 内存比较
 * @param pDst      指向目的内存的指针
 * @param uDstLen   目的内存的大小
 * @param pSrc      指向源内存的指针
 * @param uCmpLen   要比较的长度
 * @return          相等返回0
 */
static inline int32_t s_memcmp(const void * const pDst,ssize_t uDstLen,const void * const pSrc,ssize_t uCmpLen)
{
#if _MEMORY_PROTECT_
    assert(uDstLen>=uCmpLen);
    assert(uDstLen>=0 && uDstLen < MAX_ALLOC_MEMORY
           && uCmpLen >=0 && uCmpLen<MAX_ALLOC_MEMORY);
#else
    (void)uDstLen;
#endif
    return memcmp(pDst,pSrc,uCmpLen);
}

} //namespace chw

#endif // __MEMORY_HANDLE_H
