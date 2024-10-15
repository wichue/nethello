#include "Timer.h"

namespace chw {

Timer::Timer(float second, const std::function<bool()> &cb, const EventLoop::Ptr &poller) {
    _poller = poller;
    if (!_poller) {
        //_poller = EventPollerPool::Instance().getPoller();//chw:todo
    }
    _tag = _poller->doDelayTask((uint64_t) (second * 1000), [cb, second]() {
        try {
            if (cb()) {
                //重复的任务
                return (uint64_t) (1000 * second);
            }
            //该任务不再重复
            return (uint64_t) 0;
        } catch (std::exception &ex) {
            ErrorL << "Exception occurred when do timer task: " << ex.what();
            return (uint64_t) (1000 * second);
        }
    });
}

Timer::~Timer() {
    auto tag = _tag.lock();
    if (tag) {
        tag->cancel();
    }
}

}  // namespace chw
