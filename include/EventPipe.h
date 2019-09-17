#pragma once

#include <memory>
#include <mutex>

namespace ncnet {
    class EventPipe {
    public:
        explicit EventPipe();
        ~EventPipe();

        void setPipe();
        void resetPipe();

        int getSocket();

    private:
        int mPipes[2];
        std::shared_ptr<std::mutex> event_mutex_;
    };
}