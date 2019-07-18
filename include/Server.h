#ifndef SERVER_H
#define SERVER_H

#include "Network.h"
#include "Information.h"
#include "Connection.h"
#include "EventPipe.h"

#include <vector>
#include <condition_variable>

class Server : public Network {
public:
    bool start(int port);
    void run(); // Only called by internal thread handling
    void stop();

    Information get();
    bool send(const Information& information);

private:
    void prepareOutgoing();
    void prepareSets(fd_set& read_set, fd_set& write_set, fd_set& error_set);
    bool read(Connection& connection);
    bool write(Connection& connection);

    std::vector<Connection> connections_;
    EventPipe pipe_; // Needed to interrupt when adding queued packets

    std::mutex incoming_lock_;
    std::condition_variable incoming_cv_;
    std::list<Information> incoming_;
    std::mutex outgoing_lock_;
    std::condition_variable outgoing_cv_; // Not needed?
    std::list<Information> outgoing_;

    // If the server should be stopped
    std::mutex stop_lock_;
    bool stop_ = false;
};

#endif