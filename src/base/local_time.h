#ifndef UTIL_LOCALTIME_H
#define UTIL_LOCALTIME_H
#include <time.h>

namespace chw {
void no_locks_localtime(struct tm *tmp, time_t t);
void local_time_init();
int get_daylight_active();

} // namespace chw
#endif // UTIL_LOCALTIME_H
