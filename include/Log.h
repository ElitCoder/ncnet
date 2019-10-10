#pragma once

#include <iostream>
#include <sstream>
#include <mutex>

namespace ncnet {
    constexpr auto DEBUG = "DEBUG";
    constexpr auto WARN = "WARN";
    constexpr auto ERROR = "ERROR";
    constexpr auto INFO = "INFO";

    class Log : public std::ostringstream {
    public:
        Log() {}
        Log(std::string prefix) : prefix_(prefix) {}
        ~Log() {
            std::lock_guard<std::mutex> lock(lock_);
            if (!enabled_) {
                return;
            }

            if (prefix_ != "") {
                std::cout << "[" << prefix_ << "] ";
            }

            std::cout << str() << "\n" << std::flush;
        }

        static void enable(bool enabled);

    private:
        static std::mutex lock_;
        static bool enabled_;
        std::string prefix_ = "";
    };
}