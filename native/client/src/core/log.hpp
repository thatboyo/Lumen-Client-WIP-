#pragma once

#include <cstdarg>
#include <cstdio>
#include <string>
#include <mutex>
#include <windows.h>

namespace lumin {

// Minimal file logger. The overlay itself never fails loudly: if anything goes
// wrong the user gets a line in lumin.log next to the DLL.
class Log {
public:
    static void init(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex());
        if (file()) return;
        file() = fopen(path.c_str(), "w");
    }

    static void write(const char* level, const char* fmt, ...) {
        char buffer[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        SYSTEMTIME st;
        GetLocalTime(&st);

        char line[2304];
        snprintf(line, sizeof(line), "[%02d:%02d:%02d][%s] %s\n",
                 st.wHour, st.wMinute, st.wSecond, level, buffer);

        std::lock_guard<std::mutex> lock(mutex());
        if (file()) {
            fputs(line, file());
            fflush(file());
        }
        OutputDebugStringA(line);
    }

private:
    static FILE*& file() {
        static FILE* f = nullptr;
        return f;
    }

    static std::mutex& mutex() {
        static std::mutex m;
        return m;
    }
};

} // namespace lumin

#define VLOG_INFO(...) ::lumin::Log::write("info", __VA_ARGS__)
#define VLOG_WARN(...) ::lumin::Log::write("warn", __VA_ARGS__)
#define VLOG_ERR(...)  ::lumin::Log::write("error", __VA_ARGS__)
