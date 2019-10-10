#pragma once

#include <iostream>
#include <sstream>
#include <mutex>

namespace ncnet {
    // Log levels/prefixes
    enum LogLevel {
        NONE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        CRITICAL
    };

    class Log : public std::ostringstream {
    public:
        // Creates no prefix log, always shown unless fully disabled
        Log();
        // Log with prefix
        Log(LogLevel level);
        // Sync and output current stream
        ~Log();

        // Enable/disable all output
        static void enable(bool enabled);
        // Set log level
        static void set_log_level(LogLevel level);

    private:
        // Global output sync
        static std::mutex lock_;
        // Enable/disable
        static bool enabled_;
        // Log level
        static LogLevel log_level_;

        // Current output prefix
        LogLevel prefix_ = LogLevel::NONE;
    };
}