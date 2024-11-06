// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "UdpMmsg.h"
#include "SocketBase.h"

namespace chw {

/////////////////////////////////////// BufferSock ///////////////////////////////////////

BufferSock::BufferSock(Buffer::Ptr buffer, struct sockaddr *addr, int addr_len) {
    if (addr) {
        _addr_len = addr_len ? addr_len : SockUtil::get_sock_len(addr);
        memcpy(&_addr, addr, _addr_len);
    }
    assert(buffer);
    _buffer = std::move(buffer);
}

char *BufferSock::data() const {
    return (char*)_buffer->data();
}

size_t BufferSock::size() const {
    return _buffer->Size();
}

const struct sockaddr *BufferSock::sockaddr() const {
    return (struct sockaddr *)&_addr;
}

socklen_t BufferSock::socklen() const {
    return _addr_len;
}

}//namespace chw