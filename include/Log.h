#pragma once

#include <iostream>
#include <sstream>

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
            if (silent_) {
                return;
            }

            if (prefix_ != "") {
                std::cout << "[" << prefix_ << "] ";
            }

            std::cout << str() << std::endl;
        }

        static void set_silent(bool silent) { silent_ = silent; }

    private:
        static bool silent_;
        std::string prefix_ = "";
    };
}