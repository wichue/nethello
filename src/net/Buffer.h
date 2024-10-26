// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __BUFFER_H
#define __BUFFER_H

#include <memory>
#include "MemoryHandle.h"
#include "Logger.h"

namespace chw {
#define RAW_BUFFER_SIZE     2 * 1024
#define TCP_BUFFER_SIZE     128 * 1024
#define MAX_BUFFER_SIZE     16<<20       //buf最大大小,16MB

/**
 * 缓存封装，构造时缓存为空，调用 setCapacity 分配内存
 * 
 */
class Buffer {
public:
    using Ptr = std::shared_ptr<Buffer>;
    Buffer() {
        _data = nullptr;
        _size = 0;
        _rcvlen = 0;
    }

    ~Buffer() {
        if(_data != nullptr) {
            _RAM_DEL_(_data);
            _data = nullptr;
        }
    }

    // 返回buf指针
    void* data() {
        return _data;
    }

    /**
     * @brief 设置buf大小,原大小满足直接返回，原大小不足时执行扩容，并拷贝原buf数据
     * 
     * @param size [in]大小
     * @return uint32_t 成功返回chw::success，失败返回chw::fail
     */
    uint32_t setCapacity(size_t size) {
        if(size <= _size) {
            return chw::success;
        }

        void* buffer = nullptr;
        try {
            buffer = _RAM_NEW_(size);
        } catch(std::bad_alloc&) {
            PrintE("[Buffer]1 alloc failed,size=%u",size);
            return chw::fail;
        }

        if(_data != nullptr) {
            _RAM_CPY_(buffer,size,_data,_size);
            _RAM_DEL_(_data);
            _data = nullptr;
            _size = 0;
        }
        _data = buffer;
        _size = size;

        return chw::success;
    }

    /**
     * @brief 堆buf太大则释放掉，防止长时间占用大量内存
     * 
     * @param size [in]如果执行了释放，重新分配的buf大小
     * @return uint32_t 成功返回chw::success，失败返回chw::fail
     */
    uint32_t CheckBuf(size_t size) {
        if(_data != nullptr && _size > MAX_BUFFER_SIZE) {
            _RAM_DEL_(_data);
            _data = nullptr;
            _size = 0;
            _rcvlen = 0;

            try {
                _data = _RAM_NEW_(size);
                _size = size;
            } catch(std::bad_alloc&) {
                PrintE("[Buffer]2 alloc failed,size=%u",size);
                _data = nullptr;
                _size = 0;
                return chw::fail;
            }
        }

        return chw::success;
    }

    /**
     * @brief 释放缓存
     * 
     */
    void Free() {
        if(_data != nullptr)
        {
            _RAM_DEL_(_data);
            _data = nullptr;
            _size = 0;
            _rcvlen = 0;
        }
    }

    /**
     * @brief buf尾部不完整的报文拷贝到起始位置，失败时释放链路
     * 
     * @param start [in]尾部报文开始位置
     * @param end   [in]尾部报文结束位置
     * @return uint32_t 成功返回chw::success，失败返回chw::fail
     */
    uint32_t Align(size_t start, size_t end) {
        if(start >= end || end - start > _size) {
            PrintE("[Buffer] Align failed,start=%u,end=%u,_size=%u",start,end,_size);
            return chw::fail;
        }

        _RAM_CPY_((uint8_t*)_data, end - start, (uint8_t*)_data + start, end - start);
        SetRcvLen(end - start);
        return chw::success;
    }

    // 返回空闲buf大小
    size_t Idle() {
        return _size - _rcvlen;
    }

    // 返回buf总大小
    size_t Size() {
        return _size;
    }

    // 设置已接收数据长度
    void SetRcvLen(size_t len) {
        _rcvlen = len;
    }

    // 返回已接收数据大小
    size_t RcvLen() {
        return _rcvlen;
    }

    void Reset() {
        _rcvlen = 0;
    }

    void Reset0() {
        _RAM_SET_(_data,_size,0,_size);
        _rcvlen = 0;
    }

private:
    void* _data;//堆空间
    size_t _size;//堆空间大小
    size_t _rcvlen;//已接收数据大小
};

}//namespace chw
#endif //__BUFFER_H
