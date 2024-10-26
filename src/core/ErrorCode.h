// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#ifndef __ERROR_CODE_H
#define __ERROR_CODE_H

#include <stdint.h>
#include <string>
#include <unordered_map>

/**
 * 定义错误码，可根据错误码查询错误描述。
 * 
 */

#define ERRNO_MAP_FILE(XX) \
    XX(ERROR_FILE_PERM_DENIED               , 1000,  "file transf, permission denied") \
    XX(ERROR_FILE_EXIST                     , 1001,  "file already exist") \
    XX(ERROR_FILE_SEND_FAIL                 , 1002,  "file send failed") \
    XX(ERROR_FILE_INVALID_SIZE              , 1003,  "invalid file size") \
    XX(ERROR_FILE_PEER_NO_DIR               , 1004,  "file transf, peer no such directory") \
    XX(ERROR_FILE_PEER_PERM_DENIED          , 1005,  "file transf, peer permission denied") \
    XX(ERROR_FILE_INVALID_FILE_PATH         , 1006,  "file transf, invalid file path") \
    XX(ERROR_FILE_INVALID_FILE_NAME         , 1007,  "file transf, invalid file name") \
    XX(ERROR_FILE_PEER_UNKNOWN              , 1008,  "unknown peer error") \

#define ERRNO_MAP_PRESS(XX) \
    XX(ERROR_PRESS_FAIL                     , 2000,  "press failed") \

#define CHW_ERRNO_GEN(n, v, s) n = v,
enum ChwErrorCode {
#ifndef _WIN32
    ERROR_SUCCESS = 0,
#endif
    ERRNO_MAP_FILE(CHW_ERRNO_GEN)
    ERRNO_MAP_PRESS(CHW_ERRNO_GEN)
};
#undef CHW_ERRNO_GEN

namespace chw {

class ErrorCode {
public:
    ErrorCode();
    ~ErrorCode() = default;

    static ErrorCode &Instance();

    /**
     * @brief 根据错误码查询错误描述
     * 
     * @param code 错误码
     * @return std::string 错误描述
     */
    std::string error2str(uint32_t code);

private:
    std::unordered_map<int, std::string> _umCodeDesc;
};
#define Error2Str(n) ErrorCode::Instance().error2str(n)

}

#endif//__ERROR_CODE_H