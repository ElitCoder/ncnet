#include "Log.h"

namespace ncnet {
    std::mutex Log::lock_;
    bool Log::enabled_ = false;
    LogLevel Log::log_level_ = LogLevel::NONE; // Allow everything

    Log::Log() {}
    Log::Log(LogLevel level) : prefix_(level) {}
    Log::~Log() {
        std::lock_guard<std::mutex> lock(lock_);
        if (!enabled_ || prefix_ < log_level_) {
            return;
        }

        std::string prefix = "";
        switch (prefix_) {
            case LogLevel::NONE:
                break;
            case LogLevel::DEBUG: prefix = "DEBUG";
                break;
            case LogLevel::INFO: prefix = "INFORMATION";
                break;
            case LogLevel::WARN: prefix = "WARNING";
                break;
            case LogLevel::ERROR: prefix = "ERROR";
                break;
            case LogLevel::CRITICAL: prefix = "CRITICAL";
                break;
        }

        if (prefix_ != LogLevel::NONE) {
            prefix = "[" + prefix + "] ";
        }

        std::cout << prefix << str() << "\n" << std::flush;
    }

    void Log::set_log_level(LogLevel level) {
        std::lock_guard<std::mutex> lock(lock_);
        // TODO: Validate
        log_level_ = level;
    }

    void Log::enable(bool enabled) {
        std::lock_guard<std::mutex> lock(lock_);
        enabled_ = enabled;
    }
}
