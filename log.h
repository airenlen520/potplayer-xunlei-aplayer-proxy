#pragma once
#include <windows.h>
#include <cstdio>
#include <cstdarg>

// 日志文件路径：和 DLL 同目录
inline const char* GetLogPath() {
    static char path[MAX_PATH] = {0};
    if (path[0] == 0) {
        HMODULE h = nullptr;
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&GetLogPath, &h);
        GetModuleFileNameA(h, path, MAX_PATH);
        char* p = strrchr(path, '\\');
        if (p) *(p + 1) = 0;
        strcat_s(path, "APlayerProxy.log");
    }
    return path;
}

inline void ProxyLog(const char* fmt, ...) {
    FILE* f = nullptr;
    fopen_s(&f, GetLogPath(), "a");
    if (!f) return;

    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(f, "[%02d:%02d:%02d.%03d] ",
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);

    fprintf(f, "\n");
    fclose(f);
}