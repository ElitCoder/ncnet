#ifndef EVENT_PIPE_H
#define EVENT_PIPE_H

#include <memory>
#include <mutex>

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

#endif