// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __PIPE_WRAP_H
#define __PIPE_WRAP_H

#include "util.h"

namespace chw {


class TaskCancelable : public noncopyable {
public:
    TaskCancelable() = default;
    virtual ~TaskCancelable() = default;
    virtual void cancel() = 0;
};

template<class R, class... ArgTypes>
class TaskCancelableImp;

template<class R, class... ArgTypes>
class TaskCancelableImp<R(ArgTypes...)> : public TaskCancelable {
public:
    using Ptr = std::shared_ptr<TaskCancelableImp>;
    using func_type = std::function<R(ArgTypes...)>;

    ~TaskCancelableImp() = default;

    template<typename FUNC>
    TaskCancelableImp(FUNC &&task) {
        _strongTask = std::make_shared<func_type>(std::forward<FUNC>(task));
        _weakTask = _strongTask;
    }

    void cancel() override {
        _strongTask = nullptr;
    }

    operator bool() {
        return _strongTask && *_strongTask;
    }

    void operator=(std::nullptr_t) {
        _strongTask = nullptr;
    }

    R operator()(ArgTypes ...args) const {
        auto strongTask = _weakTask.lock();
        if (strongTask && *strongTask) {
            return (*strongTask)(std::forward<ArgTypes>(args)...);
        }
        return defaultValue<R>();
    }

    template<typename T>
    static typename std::enable_if<std::is_void<T>::value, void>::type
    defaultValue() {}

    template<typename T>
    static typename std::enable_if<std::is_pointer<T>::value, T>::type
    defaultValue() {
        return nullptr;
    }

    template<typename T>
    static typename std::enable_if<std::is_integral<T>::value, T>::type
    defaultValue() {
        return 0;
    }

protected:
    std::weak_ptr<func_type> _weakTask;
    std::shared_ptr<func_type> _strongTask;
};

using TaskIn = std::function<void()>;
using Task = TaskCancelableImp<void()>;

class PipeWrap {
public:
    PipeWrap();
    ~PipeWrap();
    int write(const void *buf, int n);
    int read(void *buf, int n);
    int readFD() const { return _pipe_fd[0]; }
    int writeFD() const { return _pipe_fd[1]; }
    void reOpen();

private:
    void clearFD();

private:
    int _pipe_fd[2] = { -1, -1 };
};

} //namespace chw

#endif // __PIPE_WRAP_H

