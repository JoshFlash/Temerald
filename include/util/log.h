#pragma once
#include <Windows.h>
#include <cstdio>
#include <cstdarg>

namespace util::detail {

inline void log_write(const char* prefix, const char* fmt, ...)
{
    char body[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(body, sizeof(body), fmt, args);
    va_end(args);

    char line[1152];
    snprintf(line, sizeof(line), "%s %s\n", prefix, body);

    OutputDebugStringA(line);
    fputs(line, stdout);
}

} // namespace util::detail

#define LOG_INFO(fmt, ...)  ::util::detail::log_write("[INFO] ", fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  ::util::detail::log_write("[WARN] ", fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) ::util::detail::log_write("[ERROR]", fmt, ##__VA_ARGS__)
