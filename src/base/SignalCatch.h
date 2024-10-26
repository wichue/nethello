// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef        __SIGNAL_CATCH_H
#define        __SIGNAL_CATCH_H

#include "util.h"

namespace chw {

class SignalCatch {
public:
    SignalCatch();
    ~SignalCatch();

    static SignalCatch &Instance();

    /**
     * @brief catch signal，处理用户操作导致的中断
     * 
     */
    void CustomAbort(void (*handler)(int));

    /**
     * @brief catch signal，处理非用户操作导致的中断 
     * 
     * @param handler 
     */
    void CustomCrash(void (*handler)(int));
};



}//namespace chw
#endif // __SIGNAL_CATCH_H
