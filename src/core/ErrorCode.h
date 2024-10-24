#ifndef __ERROR_CODE_H
#define __ERROR_CODE_H

#include <stdint.h>
#include <string>
#include <unordered_map>

#define ERRNO_MAP_FILE(XX) \
    XX(ERROR_FILE_PERM_DENIED               , 1000,  "Permission denied") \
    XX(ERROR_FILE_EXIST                     , 1001,  "file already exist") \
    XX(ERROR_FILE_SEND_FAIL                 , 1002,  "file send failed") \
    XX(ERROR_FILE_INVALID_SIZE              , 1003,  "invalid file size") \

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

    std::string error2str(uint32_t code);

private:
    std::unordered_map<int, std::string> _umCodeDesc;
};
#define Error2Str(n) ErrorCode::Instance().error2str(n)

}

#endif//__ERROR_CODE_H