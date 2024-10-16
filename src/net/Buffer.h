#ifndef __BUFFER_H
#define __BUFFER_H

#include <memory>
#include "MemoryHandle.h"
#include "Logger.h"

namespace chw {
#define RAW_BUFFER_SIZE     2 * 1024
#define TCP_BUFFER_SIZE     128 * 1024
#define MAX_BUFFER_SIZE     16<<20       //buf最大大小,16MB

class Buffer {
public:
    using Ptr = std::shared_ptr<Buffer>;
    Buffer() {
        _data = nullptr;
        _size = 0;
        uRcvLen = 0;
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
     * @brief 设置buf大小，失败时释放链路
     * 
     * @param size 大小
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
     * @brief 堆buf太大则释放掉，防止长时间占用大量内存，失败时释放链路
     * 
     * @param size 如果执行了释放，重新分配的buf大小
     * @return uint32_t 成功返回chw::success，失败返回chw::fail
     */
    uint32_t CheckBuf(size_t size) {
        if(_data != nullptr && _size > MAX_BUFFER_SIZE) {
            _RAM_DEL_(_data);
            _data = nullptr;
            _size = 0;
            uRcvLen = 0;

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
     * @brief buf尾部不完整的报文拷贝到起始位置，失败时释放链路
     * 
     * @param start 尾部报文开始位置
     * @param end   尾部报文结束位置
     * @return uint32_t 成功返回chw::success，失败返回chw::fail
     */
    uint32_t Align(size_t start, size_t end) {
        if(start >= end || end - start > _size) {
            PrintE("[Buffer] Align failed,start=%u,end=%u,_size=%u",start,end,_size);
            return chw::fail;
        }

        _RAM_CPY_((uint8_t*)_data, end - start, (uint8_t*)_data + start, end - start);
        return chw::success;
    }

    // 返回空闲buf大小
    size_t Idle() {
        return _size - uRcvLen;
    }

    // 返回buf总大小
    size_t Size() {
        return _size;
    }

    // 设置已接收数据长度
    void SetRcvLen(size_t len) {
        uRcvLen = len;
    }

    // 返回已接收数据大小
    size_t RcvLen() {
        return uRcvLen;
    }

    void Reset() {
        _RAM_SET_(_data,_size,0,_size);
        uRcvLen = 0;
    }

private:
    void* _data;//堆空间
    size_t _size;//堆空间大小
    size_t uRcvLen;//已接收数据大小
};

}//namespace chw
#endif //__BUFFER_H
