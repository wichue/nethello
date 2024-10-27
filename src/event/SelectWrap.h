// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __SELECT_WRAP_H
#define __SELECT_WRAP_H

#include "util.h"

namespace chw {

class FdSet {
public:
    FdSet();
    ~FdSet();
    void fdZero();
    void fdSet(int fd);
    void fdClr(int fd);
    bool isSet(int fd);
    void *_ptr;
};

int zl_select(int cnt, FdSet *read, FdSet *write, FdSet *err, struct timeval *tv);

} //namespace chw
#endif //__SELECT_WRAP_H
