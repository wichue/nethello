#include "ErrorCode.h"
#include "util.h"

namespace chw {

INSTANCE_IMP(ErrorCode)

#define SRS_STRERRNO_GEN(n, v, s) {(ChwErrorCode)v, s},
ErrorCode::ErrorCode()
{
    std::pair<int, std::string> Arr[] = {{ERROR_SUCCESS, "Success"},
                                        ERRNO_MAP_FILE(SRS_STRERRNO_GEN)
                                        ERRNO_MAP_PRESS(SRS_STRERRNO_GEN)};

    _umCodeDesc.insert(Arr,Arr + (int)(sizeof(Arr) / sizeof(Arr[0])));
}   
#undef SRS_STRERRNO_GEN

std::string ErrorCode::error2str(uint32_t code)
{
    auto iter = _umCodeDesc.find(code);
    if(iter != _umCodeDesc.end())
    {
        return iter->second;
    }

    return "unknown error";
}

}//namespace chw