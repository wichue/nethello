// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __KCP_CLIENT_H
#define __KCP_CLIENT_H

#include <memory>
#include "ikcp.h"

namespace chw {

class KcpClient : public std::enable_shared_from_this<KcpClient> {
public:
    using Ptr = std::shared_ptr<KcpClient>;
    KcpClient();
    virtual ~KcpClient() = default;

private:
    ikcpcb *_kcp;// kcp实例
};

}//namespace chw


#endif//__KCP_CLIENT_H