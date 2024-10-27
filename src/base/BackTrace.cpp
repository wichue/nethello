// Copyright (c) 2024 The nethello project authors. SPDX-License-Identifier: MIT.
// This file is part of nethello(https://github.com/wichue/nethello).

#include "BackTrace.h"
#include <errno.h>
#include <sstream>
#include <stdarg.h>
#include <assert.h>

#include <map>
#include <vector>
using namespace std;

namespace chw {

const int maxLogBuf = 4 * 1024 * 1024;

#if defined(__linux)
#include <execinfo.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>

void* parse_symbol_offset(char* frame)
{
    char* p = NULL;
    char* p_symbol = NULL;
    int nn_symbol = 0;
    char* p_offset = NULL;
    int nn_offset = 0;

    // Read symbol and offset, for example:
    //      /tools/backtrace(foo+0x1820) [0x555555555820]
    for (p = frame; *p; p++) {
        if (*p == '(') {
            p_symbol = p + 1;
        } else if (*p == '+') {
            if (p_symbol) nn_symbol = p - p_symbol;
            p_offset = p + 1;
        } else if (*p == ')') {
            if (p_offset) nn_offset = p - p_offset;
        }
    }
    if (!nn_symbol && !nn_offset) {
        return NULL;
    }

    // Convert offset(0x1820) to pointer, such as 0x1820.
    char tmp[128];
    if (!nn_offset || nn_offset >= (int)sizeof(tmp)) {
        return NULL;
    }

    int r0 = EOF;
    void* offset = NULL;
    tmp[nn_offset] = 0;
    if ((r0 = sscanf(strncpy(tmp, p_offset, nn_offset), "%p", &offset)) == EOF) {
        return NULL;
    }

    // Covert symbol(foo) to offset, such as 0x2fba.
    if (!nn_symbol || nn_symbol >= (int)sizeof(tmp)) {
        return offset;
    }

    void* object_file;
    if ((object_file = dlopen(NULL, RTLD_LAZY)) == NULL) {
        return offset;
    }

    void* address;
    tmp[nn_symbol] = 0;
    if ((address = dlsym(object_file, strncpy(tmp, p_symbol, nn_symbol))) == NULL) {
        dlclose(object_file);
        return offset;
    }

    Dl_info symbol_info;
    if ((r0 = dladdr(address, &symbol_info)) == 0) {
        dlclose(object_file);
        return offset;
    }

    dlclose(object_file);
    return (char*)symbol_info.dli_saddr - (char*)symbol_info.dli_fbase + (char*)offset;
}

const char* _chw_binary = "nethello";

char* addr2line_format(void* addr, char* symbol, char* buffer, int nn_buffer)
{
    char cmd[512] = {0};
    int r0 = snprintf(cmd, sizeof(cmd), "addr2line -C -p -s -f -a -e %s %p", _chw_binary, (void*)((char*)addr - 1));
    if (r0 < 0 || r0 >= (int)sizeof(cmd)) return symbol;

    FILE* fp = popen(cmd, "r");
    if (!fp) return symbol;

    char* p = fgets(buffer, nn_buffer, fp);
    pclose(fp);

    if (p == NULL) return symbol;
    if ((r0 = strlen(p)) == 0) return symbol;

    // Trait the last newline if exists.
    if (p[r0 - 1] == '\n') p[r0 - 1] = '\0';

    // Find symbol not match by addr2line, like
    //      0x0000000000021c87: ?? ??:0
    //      0x0000000000002ffa: _start at ??:?
    for (p = buffer; p < buffer + r0 - 1; p++) {
        if (p[0] == '?' && p[1] == '?') return symbol;
    }

    return buffer;
}
#endif

int chw_parse_asan_backtrace_symbols(char* symbol, char* out_buf)
{
#if defined(__linux)
    void* frame = parse_symbol_offset(symbol);
    if (!frame) {
        return -1;
    }

    char* fmt = addr2line_format(frame, symbol, out_buf, sizeof(out_buf));
    if (fmt != out_buf) {
        return -2;
    }

    return 0;
#endif
    return -3;
}

void chw_assert()
{
#if defined(__linux)//crash时打印回溯信息
        void* addresses[64];
        int nn_addresses = backtrace(addresses, sizeof(addresses) / sizeof(void*));
        char** symbols = backtrace_symbols(addresses, nn_addresses);

        // No error code for assert failed.
        errno = 0;

        char buffer[128];
        printf("backtrace %d frames of %s\n", nn_addresses, _chw_binary);
        for (int i = 0; i < nn_addresses; i++) {
            void* frame = parse_symbol_offset(symbols[i]);
            char* fmt = addr2line_format(frame, symbols[i], buffer, sizeof(buffer));
            int parsed = (fmt == buffer);
            printf("#%d %p %d %s\n", i, frame, parsed, fmt);
        }

        free(symbols);
#endif

    // assert(0);// 会触发SIGABRT(6)信号，导致循环调用
    exit(0);
}

}// namespace chw
