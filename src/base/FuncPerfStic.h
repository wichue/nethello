// Copyright (c) 2024 The idump project authors. SPDX-License-Identifier: MIT.
// This file is part of idump(https://github.com/wichue/idump).

#ifndef __FUNC_PERF_STIC_H_
#define __FUNC_PERF_STIC_H_

#include "Logger.h"
namespace chw {
class PerfStic {
	enum { _TIMEOUT_ = 10};// ms
public:
    PerfStic(const char* func,uint32_t uflag)
    {
		m_dStart = TimeSecond_dob();
        m_uFlag = uflag;
        strncpy(m_func, func, sizeof(m_func));
    }

    ~PerfStic()
    {
        double dEnd = TimeSecond_dob();
        uint32_t uElapse = 1000 * (dEnd - m_dStart);
       	if (uElapse >= _TIMEOUT_)
      	{
			PrintD("[PerfStic] func: %s, flag: %u, consume time: %u(ms).", m_func, m_uFlag, uElapse);									
		}
	}
private:
    char m_func[32];
    double m_dStart;
    uint32_t m_uFlag;
}; 

#define PERF_STIC(flag) PerfStic __PERFSTIC__(__FUNCTION__,flag);

} //namespace chw
#endif //__FUNC_PERF_STIC_H_
