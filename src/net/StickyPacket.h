// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __STICKY_PACKET_H
#define __STICKY_PACKET_H

#include <functional>
#include "Buffer.h"

namespace chw {

typedef std::function<void(char*,uint32_t)> DispatchCB;

/**
 * @brief tcp粘包处理
 * 
 * @param buf   [in]接收数据
 * @param cb    [in]消息分发回调
 * @return uint32_t 成功返回chw::success，发生错误返回chw::fail
 */
uint32_t StickyPacket(const Buffer::Ptr &buf,const DispatchCB& cb);

}


#endif//__STICKY_PACKET_H