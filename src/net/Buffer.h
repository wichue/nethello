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
 * 缓存封装，构造时缓存为空，调用 SetCapacity 分配内存
 * 
 */
class Buffer {
public:
    using Ptr = std::shared_ptr<Buffer>;
    Buffer() {
        _data = nullptr;
        _capacity = 0;
        _size = 0;
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
     * @brief 设置buf总大小,原大小满足直接返回，原大小不足时执行扩容，并拷贝原buf数据
     * 
     * @param size [in]大小
     * @return uint32_t 成功返回chw::success，失败返回chw::fail
     */
    uint32_t SetCapacity(size_t size) {
        if(size <= _capacity) {
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
            _RAM_CPY_(buffer,size,_data,_capacity);
            _RAM_DEL_(_data);
            _data = nullptr;
            _capacity = 0;
        }
        _data = buffer;
        _capacity = size;

        return chw::success;
    }

    /**
     * @brief 堆buf太大则释放掉，防止长时间占用大量内存
     * 
     * @param size [in]如果执行了释放，重新分配的buf大小
     * @return uint32_t 成功返回chw::success，失败返回chw::fail
     */
    uint32_t CheckBuf(size_t size) {
        if(_data != nullptr && _capacity > MAX_BUFFER_SIZE) {
            _RAM_DEL_(_data);
            _data = nullptr;
            _capacity = 0;
            _size = 0;

            try {
                _data = _RAM_NEW_(size);
                _capacity = size;
            } catch(std::bad_alloc&) {
                PrintE("[Buffer]2 alloc failed,size=%u",size);
                _data = nullptr;
                _capacity = 0;
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
            _capacity = 0;
            _size = 0;
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
        if(start >= end || end - start > _capacity) {
            PrintE("[Buffer] Align failed,start=%u,end=%u,_capacity=%u",start,end,_capacity);
            return chw::fail;
        }

        _RAM_CPY_((uint8_t*)_data, end - start, (uint8_t*)_data + start, end - start);
        SetSize(end - start);
        return chw::success;
    }

    // 返回空闲buf大小
    size_t Idle() {
        return _capacity - _size;
    }

    // 返回buf总大小
    size_t Capacity() {
        return _capacity;
    }

    // 设置有效数据长度
    void SetSize(size_t len) {
        _size = len;
    }

    // 返回有效数据大小
    size_t Size() {
        return _size;
    }

    void Reset() {
        _size = 0;
    }

    void Reset0() {
        _RAM_SET_(_data,_capacity,0,_capacity);
        _size = 0;
    }

private:
    void* _data;//堆空间
    size_t _capacity;//堆空间总大小
    size_t _size;//有效数据大小
};

}//namespace chw
#endif //__BUFFER_H
