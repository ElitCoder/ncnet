#include "Log.h"

namespace ncnet {
    std::mutex Log::lock_;
    bool Log::enabled_ = false;

    void Log::enable(bool enabled) {
        std::lock_guard<std::mutex> lock(lock_);
        enabled_ = enabled;
    }
}
