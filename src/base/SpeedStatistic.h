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
    BytesSpeed &operator+=(size_t bytes) {
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
    int getSpeed() {
        if (_ticker.elapsedTime() < 1000) {
            //获取频率小于1秒，那么返回上次计算结果
            return _speed;
        }
        return computeSpeed();
    }

private:
    int computeSpeed() {
        auto elapsed = _ticker.elapsedTime();
        if (!elapsed) {
            return _speed;
        }
        _speed = (int)(_bytes * 1000 / elapsed);
        _ticker.resetTime();
        _bytes = 0;
        return _speed;
    }

private:
    int _speed = 0;
    size_t _bytes = 0;
    Ticker _ticker;
};

} //namespace chw

#endif //__SPEED_STATISTIC_H
