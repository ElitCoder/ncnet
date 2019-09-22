#pragma once

#include <memory>
#include <mutex>

namespace ncnet {
    class EventPipe {
    public:
        explicit EventPipe();
        ~EventPipe();

        void activate(); // Trigger pipe socket
        void reset(); // Reset pipe socket

        int get_socket(); // Returns writable pipe

    private:
        int pipes_[2] = { -1, -1 };
        std::shared_ptr<std::mutex> event_mutex_;
    };
}