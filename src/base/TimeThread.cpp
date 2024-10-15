#include "TimeThread.h"
#include <atomic>
#include <thread>
#include "Logger.h"
#include "onceToken.h"
#include "local_time.h"

using namespace std;
namespace chw {

static long s_gmtoff = 0; //时间差
static onceToken s_token([]() {
#ifdef _WIN32
    TIME_ZONE_INFORMATION tzinfo;
    DWORD dwStandardDaylight;
    long bias;
    dwStandardDaylight = GetTimeZoneInformation(&tzinfo);
    bias = tzinfo.Bias;
    if (dwStandardDaylight == TIME_ZONE_ID_STANDARD) {
        bias += tzinfo.StandardBias;
    }
    if (dwStandardDaylight == TIME_ZONE_ID_DAYLIGHT) {
        bias += tzinfo.DaylightBias;
    }
    s_gmtoff = -bias * 60; //时间差(分钟)
#else
    local_time_init();
    s_gmtoff = getLocalTime(time(nullptr)).tm_gmtoff;
#endif // _WIN32
});

long getGMTOff() {
    return s_gmtoff;
}

static inline uint64_t getCurrentMicrosecondOrigin() {
#if !defined(_WIN32)
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
#else
    return  std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#endif
}

static atomic<uint64_t> s_currentMicrosecond(0);
static atomic<uint64_t> s_currentMillisecond(0);
static atomic<uint64_t> s_currentMicrosecond_system(getCurrentMicrosecondOrigin());
static atomic<uint64_t> s_currentMillisecond_system(getCurrentMicrosecondOrigin() / 1000);

static inline bool initMillisecondThread() {
    static std::thread s_thread([]() {
        setThreadName("time thread");
        DebugL << "time thread started";
        uint64_t last = getCurrentMicrosecondOrigin();
        uint64_t now;
        uint64_t microsecond = 0;
        while (true) {
            now = getCurrentMicrosecondOrigin();
            //记录系统时间戳，可回退  [AUTO-TRANSLATED:495a0114]
            //Record system timestamp, can be rolled back
            s_currentMicrosecond_system.store(now, memory_order_release);
            s_currentMillisecond_system.store(now / 1000, memory_order_release);

            //记录流逝时间戳，不可回退  [AUTO-TRANSLATED:7f3a9da3]
            //Record elapsed timestamp, cannot be rolled back
            int64_t expired = now - last;
            last = now;
            if (expired > 0 && expired < 1000 * 1000) {
                //流逝时间处于0~1000ms之间，那么是合理的，说明没有调整系统时间  [AUTO-TRANSLATED:566e1001]
                //If the elapsed time is between 0~1000ms, it is reasonable, indicating that the system time has not been adjusted
                microsecond += expired;
                s_currentMicrosecond.store(microsecond, memory_order_release);
                s_currentMillisecond.store(microsecond / 1000, memory_order_release);
            } else if (expired != 0) {
                WarnL << "Stamp expired is abnormal: " << expired;
            }
            //休眠0.5 ms  [AUTO-TRANSLATED:5e20acdd]
            //Sleep for 0.5 ms
            usleep(500);
        }
    });
    static onceToken s_token([]() {
        s_thread.detach();
    });
    return true;
}

uint64_t getCurrentMillisecond(bool system_time) {
    static bool flag = initMillisecondThread();
    (void)flag;
    if (system_time) {
        return s_currentMillisecond_system.load(memory_order_acquire);
    }
    return s_currentMillisecond.load(memory_order_acquire);
}

uint64_t getCurrentMicrosecond(bool system_time) {
    static bool flag = initMillisecondThread();
    (void)flag;
    if (system_time) {
        return s_currentMicrosecond_system.load(memory_order_acquire);
    }
    return s_currentMicrosecond.load(memory_order_acquire);
}

}//namespace chw