// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __BUFFER_H
#define __BUFFER_H

#include <memory>
#include "MemoryHandle.h"
#include "BackTrace.h"

namespace chw {

/**
 * 缓存封装，栈空间大小固定，优先使用栈空间，大于栈空间时使用堆空间。
 * 
 * @tparam StackSize    栈空间大小
 * @tparam MaxHeapSize  堆空间最大值
 */
template <size_t StackSize, size_t MaxHeapSize>
class BufferAuto {
public:
    BufferAuto() {
        _heap = nullptr;
        _heapsize = 0;
        uRcvLen = 0;
    }

    ~BufferAuto() {
        if(_heap != nullptr) {
            _RAM_DEL_(_heap);
            _heap = nullptr;
        }
    }

    // 返回buf指针
    void* Data(size_t size) {
        // 优先使用栈空间
        if(size <= StackSize) {
            return (void*)_stack;
        }

        if(size > _heapsize) {
            // 堆空间不足则执行扩容
            if(_heap) {
                void* buffer = _RAM_NEW_(size);
                if(buffer != nullptr) {
                    _RAM_CPY_(buffer,size,_heap,_heapsize);
                    _RAM_DEL_(_heap);
                    _heap = nullptr;
                    _heap = buffer;
                    _heapsize = size;
                }
                else {
                    chw_assert();
                }
            // 首次分配堆空间，把栈空间数据拷贝过来
            } else {
                _heap = _RAM_NEW_(size);
                if(_heap != nullptr) {
                    _RAM_CPY_(_heap,size,(void*)_stack,StackSize);
                    _heapsize = size;
                } else {
                    chw_assert();
                }
            }
        }

        return _heap;
    }

    // 堆buf太大则释放掉，防止长时间占有大量内存
    void CheckBuf() {
        if(_heap != nullptr && _heapsize > MaxHeapSize) {
            _RAM_DEL_(_heap);
            _heap = nullptr;
            _heapsize = 0;
        }
    }

    // 返回buf总大小
    size_t Size() {
        return _heapsize ? _heapsize : StackSize;
    }

    // 返回已接收数据大小
    size_t RcvLen() {
        return uRcvLen;
    }

private:
    char _stack[StackSize];//栈空间
    void* _heap;//堆空间
    size_t _heapsize;//堆空间大小

    size_t uRcvLen;//已接收数据大小
};
// typedef Buffer<1024*2, 128<<20> SocketBuf;
// using SocketBufPtr = std::shared_ptr<Buffer<1024*2, 128<<20>>;

}//namespace chw
#endif //__BUFFER_H
