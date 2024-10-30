// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).


#include "util.h"
#include <assert.h>
#include <limits.h>//for PATH_MAX
#if defined(__linux__) || defined(__linux)
#include <regex.h>
#endif// defined(__linux__) || defined(__linux)
#include <algorithm>
#include <iostream>
#include <regex>
#include <string>

#if defined(_WIN32)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
extern "C" const IMAGE_DOS_HEADER __ImageBase;
#endif // defined(_WIN32)

#include <string.h>
#include <string>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <algorithm>

#include "local_time.h"
#include "Logger.h"
#include "MemoryHandle.h"
#include "uv_errno.h"
#include "File.h"

namespace chw {


#if defined(_WIN32)
    void sleep(int second) {
        Sleep(1000 * second);
    }
    void usleep(int micro_seconds) {
        std::this_thread::sleep_for(std::chrono::microseconds(micro_seconds));
    }

    int gettimeofday(struct timeval* tp, void* tzp) {
        auto now_stamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        tp->tv_sec = (decltype(tp->tv_sec))(now_stamp / 1000000LL);
        tp->tv_usec = now_stamp % 1000000LL;
        return 0;
    }

    const char* strcasestr(const char* big, const char* little) {
        std::string big_str = big;
        std::string little_str = little;
        strToLower(big_str);
        strToLower(little_str);
        auto pos = strstr(big_str.data(), little_str.data());
        if (!pos) {
            return nullptr;
        }
        return big + (pos - big_str.data());
    }

    int vasprintf(char** strp, const char* fmt, va_list ap) {
        // _vscprintf tells you how big the buffer needs to be
        int len = _vscprintf(fmt, ap);
        if (len == -1) {
            return -1;
        }
        size_t size = (size_t)len + 1;
        char* str = (char*)malloc(size);
        if (!str) {
            return -1;
        }
        // _vsprintf_s is the "secure" version of vsprintf
        int r = vsprintf_s(str, len + 1, fmt, ap);
        if (r == -1) {
            free(str);
            return -1;
        }
        *strp = str;
        return r;
    }

    int asprintf(char** strp, const char* fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        int r = vasprintf(strp, fmt, ap);
        va_end(ap);
        return r;
    }

#endif //WIN32

static std::string limitString(const char *name, size_t max_size) {
    std::string str = name;
    if (str.size() + 1 > max_size) {
        auto erased = str.size() + 1 - max_size + 3;
        str.replace(5, erased, "...");
    }
    return str;
}

/**
 * @brief Set the Thread Name object
 * 
 * @param name 
 */
void setThreadName(const char *name) {
    assert(name);
#if defined(__linux) || defined(__linux__) || defined(__MINGW32__)
    pthread_setname_np(pthread_self(), limitString(name, 16).data());
#elif defined(__MACH__) || defined(__APPLE__)
    pthread_setname_np(limitString(name, 32).data());
#elif defined(_MSC_VER)
    // SetThreadDescription was added in 1607 (aka RS1). Since we can't guarantee the user is running 1607 or later, we need to ask for the function from the kernel.
    using SetThreadDescriptionFunc = HRESULT(WINAPI * )(_In_ HANDLE hThread, _In_ PCWSTR lpThreadDescription);
    static auto setThreadDescription = reinterpret_cast<SetThreadDescriptionFunc>(::GetProcAddress(::GetModuleHandle((LPCWSTR)"Kernel32.dll"), "SetThreadDescription"));
    if (setThreadDescription) {
        // Convert the thread name to Unicode
        wchar_t threadNameW[MAX_PATH];
        size_t numCharsConverted;
        errno_t wcharResult = mbstowcs_s(&numCharsConverted, threadNameW, name, MAX_PATH - 1);
        if (wcharResult == 0) {
            HRESULT hr = setThreadDescription(::GetCurrentThread(), threadNameW);
            if (!SUCCEEDED(hr)) {
                int i = 0;
                i++;
            }
        }
    } else {
        // For understanding the types and values used here, please see:
        // https://docs.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code

        const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push, 8)
        struct THREADNAME_INFO {
            DWORD dwType = 0x1000; // Must be 0x1000
            LPCSTR szName;         // Pointer to name (in user address space)
            DWORD dwThreadID;      // Thread ID (-1 for caller thread)
            DWORD dwFlags = 0;     // Reserved for future use; must be zero
        };
#pragma pack(pop)

        THREADNAME_INFO info;
        info.szName = name;
        info.dwThreadID = (DWORD) - 1;

        __try{
                RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), reinterpret_cast<const ULONG_PTR *>(&info));
        } __except(GetExceptionCode() == MS_VC_EXCEPTION ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_EXECUTE_HANDLER) {
        }
    }
#else
    thread_name = name ? name : "";
#endif
}

bool setPriority(Priority priority, std::thread::native_handle_type threadId) {
        // set priority
#if defined(_WIN32)
        static int Priorities[] = { THREAD_PRIORITY_LOWEST, THREAD_PRIORITY_BELOW_NORMAL, THREAD_PRIORITY_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_HIGHEST };
        if (priority != PRIORITY_NORMAL && SetThreadPriority(GetCurrentThread(), Priorities[priority]) == 0) {
            return false;
        }
        return true;
#else
        static int Min = sched_get_priority_min(SCHED_FIFO);
        if (Min == -1) {
            return false;
        }
        static int Max = sched_get_priority_max(SCHED_FIFO);
        if (Max == -1) {
            return false;
        }
        static int Priorities[] = {Min, Min + (Max - Min) / 4, Min + (Max - Min) / 2, Min + (Max - Min) * 3 / 4, Max};

        if (threadId == 0) {
            threadId = pthread_self();
        }
        struct sched_param params;
        params.sched_priority = Priorities[priority];
        return pthread_setschedparam(threadId, SCHED_FIFO, &params) == 0;
#endif
}

bool setThreadAffinity(int i) {
#if (defined(__linux) || defined(__linux__)) && !defined(ANDROID)
    cpu_set_t mask;
    CPU_ZERO(&mask);
    if (i >= 0) {
        CPU_SET(i, &mask);
    } else {
        for (auto j = 0u; j < std::thread::hardware_concurrency(); ++j) {
            CPU_SET(j, &mask);
        }
    }
    if (!pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask)) {
        return true;
    }
    WarnL << "pthread_setaffinity_np failed: " << get_uv_errmsg();
#endif
    return false;
}

/**
 * @brief Get the Thread Name object
 * 
 * @return std::string 
 */
std::string getThreadName() {
#if ((defined(__linux) || defined(__linux__)) && !defined(ANDROID)) || (defined(__MACH__) || defined(__APPLE__)) || (defined(ANDROID) && __ANDROID_API__ >= 26) || defined(__MINGW32__)
    std::string ret;
    ret.resize(32);
    auto tid = pthread_self();
    pthread_getname_np(tid, (char *) ret.data(), ret.size());
    if (ret[0]) {
        ret.resize(strlen(ret.data()));
        return ret;
    }
    return std::to_string((uint64_t) tid);
#elif defined(_MSC_VER)
    using GetThreadDescriptionFunc = HRESULT(WINAPI * )(_In_ HANDLE hThread, _In_ PWSTR * ppszThreadDescription);
    static auto getThreadDescription = reinterpret_cast<GetThreadDescriptionFunc>(::GetProcAddress(::GetModuleHandleA("Kernel32.dll"), "GetThreadDescription"));

    if (!getThreadDescription) {
        std::ostringstream ss;
        ss << std::this_thread::get_id();
        return ss.str();
    } else {
        PWSTR data;
        HRESULT hr = getThreadDescription(GetCurrentThread(), &data);
        if (SUCCEEDED(hr) && data[0] != '\0') {
            char threadName[MAX_PATH];
            size_t numCharsConverted;
            errno_t charResult = wcstombs_s(&numCharsConverted, threadName, data, MAX_PATH - 1);
            if (charResult == 0) {
                LocalFree(data);
                std::ostringstream ss;
                ss << threadName;
                return ss.str();
            } else {
                if (data) {
                    LocalFree(data);
                }
                return std::to_string((uint64_t) GetCurrentThreadId());
            }
        } else {
            if (data) {
                LocalFree(data);
            }
            return std::to_string((uint64_t) GetCurrentThreadId());
        }
    }
#else
    if (!thread_name.empty()) {
        return thread_name;
    }
    std::ostringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
#endif
}

/**
 * @brief TimeSecond
 * @return 从1970年到现在过去的秒数
 */
time_t TimeSecond() {
    time_t t;
    time(&t);
    return t;
}

/**
 * @brief TimeSecond
 * @return 从1970年到现在过去的秒数,精确到6位小数点，微秒
 */
double TimeSecond_dob() {
    struct timeval _time;
    gettimeofday(&_time, NULL);
    return (double)(_time.tv_sec * 1000*1000 + _time.tv_usec) /  (1000*1000);
}

/**
 * 根据unix时间戳获取本地时间
 * @param [in]sec unix时间戳
 * @return tm结构体
 */
struct tm getLocalTime(time_t sec) {
    struct tm tm;
#ifdef _WIN32
    localtime_s(&tm, &sec);
#else
    no_locks_localtime(&tm, sec);
#endif //_WIN32
    return tm;
}

/**
 * 获取时间字符串
 * @param fmt [in]时间格式，譬如%Y-%m-%d %H:%M:%S
 * @return 时间字符串
 */
std::string getTimeStr(const char *fmt, time_t time) {
    if (!time) {
        time = ::time(nullptr);
    }
    auto tm = getLocalTime(time);
    size_t size = strlen(fmt) + 64;
    std::string ret;
    ret.resize(size);
    size = strftime(&ret[0], size, fmt, &tm);
    if (size > 0) {
        ret.resize(size);
    }
    else{
        ret = fmt;
    }
    return ret;
}

bool start_with(const std::string &str, const std::string &substr) {
    return str.find(substr) == 0;
}

bool end_with(const std::string &str, const std::string &substr) {
    auto pos = str.rfind(substr);
    return pos != std::string::npos && pos == str.size() - substr.size();
}

/**
 * @brief 内存mac地址转换为字符串
 * 
 * @param macAddress    [in]mac地址buf
 * @return std::string  :分mac地址字符串
 */
std::string MacBuftoStr(const unsigned char* mac_buf) {
    char str[32] = {0};
    sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac_buf[0], mac_buf[1], mac_buf[2],
            mac_buf[3], mac_buf[4], mac_buf[5]);

    return str;
}

bool isValidMacAddress(const std::string& mac) {
    // Regular expression for MAC address
    std::regex macRegex(
        "^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$"
    );

    // Check if the string matches the MAC address regex
    return std::regex_match(mac, macRegex);
}

/**
 * @brief :分mac地址字符串转换为6字节buf
 * 
 * @param charArray     [in]:分mac地址字符串
 * @param macAddress    [out]6字节长度buf
 * @return uint32_t 成功返回chw::success,失败返回chw::fail
 */
uint32_t StrtoMacBuf(const char* charArray, unsigned char* macAddress) {
    if(isValidMacAddress(charArray) == chw::fail)
    {
        return chw::fail;
    }

    std::istringstream iss(charArray);
    int value;

    for (int i = 0; i < 6; i++) {
        iss >> std::hex >> value;
        macAddress[i] = static_cast<unsigned char>(value);
        iss.ignore(1, ':');
    }

    return chw::success;
}

#if defined(__linux__) || defined(__linux)
/**
 * @brief 判断字符串是否有效的mac地址
 * 
 * @param mac   [in]字符串
 * @return uint32_t 成功返回chw::success,失败返回chw::fail
 */
uint32_t is_valid_mac_addr(const char* mac) {
    int status;
    const char * pattern = "^([A-Fa-f0-9]{2}[-,:]){5}[A-Fa-f0-9]{2}$";
    const int cflags = REG_EXTENDED | REG_NEWLINE;

    char ebuf[128];
    regmatch_t pmatch[10];
    int nmatch = 10;
    regex_t reg;


    status = regcomp(&reg, pattern, cflags);//编译正则模式
    if(status != 0) {
        regerror(status, &reg, ebuf, sizeof(ebuf));
        PrintD( "error:regcomp fail: %s , pattern '%s' \n",ebuf, pattern);
        goto failed;
    }

    status = regexec(&reg, mac, nmatch, pmatch,0);//执行正则表达式和缓存的比较,
    if(status != 0) {
        regerror(status, &reg, ebuf, sizeof(ebuf));
        PrintD( "error:regexec fail: %s , mac:\"%s\" \n", ebuf, mac);
        goto failed;
    }

    //PrintD("[%s] match success.\n", __FUNCTION__);
    regfree(&reg);
    return chw::success;

failed:
    regfree(&reg);
    return chw::fail;
}
#endif// defined(__linux__) || defined(__linux)

std::string exePath(bool isExe /*= true*/) {
    char buffer[PATH_MAX * 2 + 1] = {0};
    int n = -1;
#if defined(_WIN32)
    n = GetModuleFileNameA(isExe?nullptr:(HINSTANCE)&__ImageBase, buffer, sizeof(buffer));
#elif defined(__MACH__) || defined(__APPLE__)
    n = sizeof(buffer);
    if (uv_exepath(buffer, &n) != 0) {
        n = -1;
    }
#elif defined(__linux__)
    n = readlink("/proc/self/exe", buffer, sizeof(buffer));
#endif

    std::string filePath;
    if (n <= 0) {
        filePath = "./";
    } else {
        filePath = buffer;
    }

#if defined(_WIN32)
    //windows下把路径统一转换层unix风格，因为后续都是按照unix风格处理的
    for (auto &ch : filePath) {
        if (ch == '\\') {
            ch = '/';
        }
    }
#endif //defined(_WIN32)

    return filePath;
}

std::string exeDir(bool isExe /*= true*/) {
    auto path = exePath(isExe);
    return path.substr(0, path.rfind('/') + 1);
}

std::string exeName(bool isExe /*= true*/) {
    auto path = exePath(isExe);
    return path.substr(path.rfind('/') + 1);
}

#define _strrchr_dot(str) strrchr(str, '.')
const char* suffixname(const char* filename) {
    const char* pos = _strrchr_dot(filename);
    return pos ? pos+1 : "";
}

// string转小写
std::string& strToLower(std::string& str) {
    transform(str.begin(), str.end(), str.begin(), towlower);
    return str;
}

// string转大写
std::string& strToUpper(std::string& str) {
    transform(str.begin(), str.end(), str.begin(), towupper);
    return str;
}

// string转小写
std::string strToLower(std::string&& str) {
    transform(str.begin(), str.end(), str.begin(), towlower);
    return std::move(str);
}

// string转大写
std::string strToUpper(std::string&& str) {
    transform(str.begin(), str.end(), str.begin(), towupper);
    return std::move(str);
}

/**
 * @brief 统计字符串中包含某字符的数量
 * 
 * @param msg		[in]字符串
 * @param c			[in]字符
 * @return 字符数量
 */
uint32_t count_char(const std::string& msg, char c)
{
	return std::count(msg.begin(), msg.end(), c);
}

/**
 * @brief 字符串分割
 * 
 * @param s        [in]输入字符串
 * @param delim    [in]分隔符
 * @return std::vector<std::string> 分割得到的子字符串集
 */
std::vector<std::string> split(const std::string &s, const char *delim) {
    std::vector<std::string> ret;
    size_t last = 0;
    auto index = s.find(delim, last);
    while (index != std::string::npos) {
        if (index - last > 0) {
            ret.push_back(s.substr(last, index - last));
        }
        last = index + strlen(delim);
        index = s.find(delim, last);
    }
    if (!s.size() || s.size() - last > 0) {
        ret.push_back(s.substr(last));
    }
    return ret;
}

/**
 * @brief 判断字符串是否为空
 * @param value [in]入参字符串
 * @return true字符串为空，false字符串不为空
 */
bool StrIsNull(const char *value)
{
    if(!value || value[0] == '\0')
    {
        return true;
    }

    return false;
}

/**
 * @brief 将16进制字符转换为10进制
 * @param hex   [in]16进制字符
 * @return      转换后的10进制
 */
unsigned char HextoInt(unsigned char hex)
{
    const int DEC = 10;
    if(('0' <= hex) && ('9' >= hex))
    {
        //减去'0'转换为整数
        return (hex - '0');
    }
    else if(('A' <= hex) && ('F' >= hex))
    {
        return (hex - 'A' + DEC);
    }
    else if(('a' <= hex) && ('f' >= hex))
    {
        return (hex - 'a' + DEC);
    }

    return 0;
}

/**
 * @brief 16进制表示字符串转换成16进制内存buf ("0080"字符串 -> 查看内存是0080)
 * @param value [in]要转换的字符串
 * @return      返回转换的结果
 */
std::string StrHex2StrBuf(const char *value)
{
    if(StrIsNull(value))
    {
        return "";
    }

    //2个字符表示一个16进制数
    int len = strlen(value);
    if(len % 2 != 0)
    {
        return "";
    }

    std::string result(len / 2, '\0');
    for(int i = 0; i < len; i += 2)
    {
        result[i / 2] = HextoInt(value[i]) * 16 + HextoInt(value[i + 1]);
    }

    return result;
}

std::string StrHex2StrBuf(const char *value, char wild_card)
{
    if(StrIsNull(value))
    {
        return "";
    }

    //2个字符表示一个16进制数
    int len = strlen(value);
    if(len % 2 != 0)
    {
        return "";
    }

    std::string result(len / 2, '\0');
    for(int i = 0; i < len; i += 2)
    {
        if(value[i] == wild_card)
        {
            result[i / 2] = wild_card;
        }
        else
        {
            result[i / 2] = HextoInt(value[i]) * 16 + HextoInt(value[i + 1]);
        }
        
    }

    return result;
}

/**
 * @brief 将内存buffer转换成16进制形式字符串(内存16进制0800->"0800"字符串)
 * @param value [in]buffer
 * @param len   [in]长度
 * @return      转换后的字符串
 */
std::string HexBuftoString(const unsigned char *value, int len)
{
    std::string result(len * 2, '\0');
    for(int i = 0;i < len; ++i)
    {
        char *buff = (char *)result.data() + i * 2;
        sprintf(buff ,"%02x", value[i]);
    }

    return std::move(result);
}

/**
 * @brief 替换字符串中的子串
 * 
 * @param str   [in]字符串
 * @param find  [in]子串
 * @param rep   [in]替换为的串
 * @return std::string 替换后的字符串
 */
std::string replaceAll(const std::string& str, const std::string& find, const std::string& rep)
{
    std::string::size_type pos = 0;
    std::string::size_type a = find.size();
    std::string::size_type b = rep.size();

    std::string res(str);
    while ((pos = res.find(find, pos)) != std::string::npos) {
        res.replace(pos, a, rep);
        pos += b;
    }
    return res;
}

/**
 * @brief 取4字节整数的低4位，高28位补0，示例:100(...0110 0100)——>4(...0000 0100)
 * 
 * @param num	[in]输入整数
 * @return 	    转换后的整数
 */
int32_t int32_lowfour(int32_t num)
{
	return num & 0xF;
}

/**
 * @brief 取4字节整数的高4位，低28位补0
 * 
 * @param num	[in]输入整数
 * @return 	    转换后的整数
 */
int32_t int32_highfour(int32_t num)
{
	return ((num >> 28) & 0xF);
}

/**
 * @brief 取2字节整数的低4位，高12位补0
 * 
 * @param num	[in]输入整数
 * @return 	    转换后的整数
 */
int16_t int16_lowfour(int16_t num)
{
	return num & 0xF;
}

/**
 * @brief 取2字节整数的高4位，低12位补0
 * 
 * @param num	[in]输入整数
 * @return 	    转换后的整数
 */
int16_t int16_highfour(int16_t num)
{
    return ((num >> 12) & 0xF);
}

/**
 * @brief 取1字节整数的低4位，高4位补0
 * 
 * @param num	[in]输入整数
 * @return 	    转换后的整数
 */
int8_t int8_lowfour(int8_t num)
{
    return num & 0xF;
}

/**
 * @brief 取1字节整数的高4位，低4位补0
 * 
 * @param num	[in]输入整数
 * @return 	    转换后的整数
 */
int8_t int8_highfour(int8_t num)
{
    return ((num >> 4) & 0xF);
}

/**
 * @brief 字节/秒的速率转换为方便识别的大单位速率
 * 
 * @param BytesPs   [in]速率，字节/秒
 * @param speed     [out]速率
 * @param unit      [out]速率单位
 */
void speed_human(uint64_t BytesPs, double& speed, std::string& unit)
{
    if(BytesPs < 1024)
    {
        speed = BytesPs;
        unit = "Bytes/s";
    }
    else if(BytesPs >= 1024 && BytesPs < 1024 * 1024)
    {
        speed = (double)BytesPs / 1024;
        unit = "KB/s";
    }
    else if(BytesPs >= 1024 * 1024 && BytesPs < 1024 * 1024 * 1024)
    {
        speed = (double)BytesPs / 1024 / 1024;
        unit = "MB/s";
    }
    else
    {
        speed = (double)BytesPs / 1024 / 1024 / 1024;
        unit = "GB/s";
    }
}

#ifndef HAS_CXA_DEMANGLE
// We only support some compilers that support __cxa_demangle.
// TODO: Checks if Android NDK has fixed this issue or not.
#if defined(__ANDROID__) && (defined(__i386__) || defined(__x86_64__))
#define HAS_CXA_DEMANGLE 0
#elif (__GNUC__ >= 4 || (__GNUC__ >= 3 && __GNUC_MINOR__ >= 4)) && \
    !defined(__mips__)
#define HAS_CXA_DEMANGLE 1
#elif defined(__clang__) && !defined(_MSC_VER)
#define HAS_CXA_DEMANGLE 1
#else
#define HAS_CXA_DEMANGLE 0
#endif
#endif
#if HAS_CXA_DEMANGLE
#include <cxxabi.h>
#endif

std::string demangle(const char *mangled) {
    int status = 0;
    char *demangled = nullptr;
#if HAS_CXA_DEMANGLE
    demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
#endif
    std::string out;
    if (status == 0 && demangled) { // Demangling succeeeded.
        out.append(demangled);
#ifdef ASAN_USE_DELETE
        delete [] demangled; // 开启asan后，用free会卡死
#else
        free(demangled);
#endif
    } else {
        out.append(mangled);
    }
    return out;
}

} /* namespace chw */
