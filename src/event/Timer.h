// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __TIMER_H
#define __TIMER_H

#include <functional>
#include "EventLoop.h"

namespace chw {

class Timer {
public:
    using Ptr = std::shared_ptr<Timer>;

    /**
     * 构造定时器
     * @param second 定时器重复秒数
     * @param cb 定时器任务，返回true表示重复下次任务，否则不重复，如果任务中抛异常，则默认重复下次任务
     * @param poller EventPoller对象，可以为nullptr
     */
    Timer(float second, const std::function<bool()> &cb, const EventLoop::Ptr &poller);
    ~Timer();

private:
    std::weak_ptr<EventLoop::DelayTask> _tag;
    //定时器保持EventPoller的强引用
    EventLoop::Ptr _poller;
};

}  // namespace chw
#endif //__TIMER_H
