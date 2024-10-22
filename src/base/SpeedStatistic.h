#ifndef __SPEED_STATISTIC_H
#define __SPEED_STATISTIC_H

#include "TimeTicker.h"

namespace chw {

class BytesSpeed {
public:
    BytesSpeed() = default;
    ~BytesSpeed() = default;

    /**
     * 添加统计字节
     */
    BytesSpeed &operator+=(uint64_t bytes) {
        _bytes += bytes;
#if 0//chw:只在需要时计算速率
        if (_bytes > 1024 * 1024) {
            //数据大于1MB就计算一次网速
            computeSpeed();
        }
#endif
        return *this;
    }

    /**
     * 获取速度，单位bytes/s
     */
    uint64_t getSpeed() {
#if 0
        if (_ticker.elapsedTime() < 1000) {
            //获取频率小于1秒，那么返回上次计算结果
            return _speed;
        }
#endif
        return computeSpeed();
    }

private:
    uint64_t computeSpeed() {
        auto elapsed = _ticker.elapsedTime();
        PrintD("elapsed=%u",elapsed);
        if (elapsed == 0) {
            return _speed;
        }
        _speed = ((_bytes - _bytes_last) * 1000 / elapsed);
        PrintD("_speed=%lu,_bytes=%lu,_bytes_last=%lu",_speed,_bytes,_bytes_last);
        _ticker.resetTime();
        // _bytes = 0;
        _bytes_last = _bytes;
        return _speed;
    }

private:
    uint64_t _speed = 0;
    uint64_t _bytes = 0;
    uint64_t _bytes_last = 0;// 上一次统计的字节数
    Ticker _ticker;
};

} //namespace chw

#endif //__SPEED_STATISTIC_H
