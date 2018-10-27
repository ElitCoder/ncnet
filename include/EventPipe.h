#ifndef EVENT_PIPE_H
#define EVENT_PIPE_H

#include <memory>
#include <mutex>

class EventPipe {
public:
    explicit EventPipe();
    ~EventPipe();

    void set();
    void reset();

    int get_socket();

private:
    int pipes_[2];
    std::shared_ptr<std::mutex> event_mutex_;
};

#endif