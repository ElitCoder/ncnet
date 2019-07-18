#ifndef NETWORK_H
#define NETWORK_H

#include "Information.h"
#include "Connection.h"
#include "EventPipe.h"

#include <thread>
#include <vector>
#include <condition_variable>

class Network {
public:
    // Override from sub-classes
    virtual bool start(const std::string &hostname, int port) = 0;
    // Called by sub-classes
    bool prepareSocket(int fd);
    int getSocket() const;
    void run(); // Only called by internal thread handling
    void stop();

    Information get();
    //bool send(const Information& information);

protected:
    // Main network thread
    std::thread network_;
    int socket_;
    bool is_client_ = false;

    std::vector<Connection> connections_;
    EventPipe pipe_; // Needed to interrupt when adding queued packets
    std::mutex outgoing_lock_;
    std::list<Information> outgoing_;

private:
    // Moves outgoing packets to the correct connection queue
    void prepareOutgoing();
    // Selects sockets to listen on
    void prepareSets(fd_set& read_set, fd_set& write_set, fd_set& error_set);
    // Read from connection
    bool read(Connection& connection);
    // Write to connection
    bool write(Connection& connection);

    std::mutex incoming_lock_;
    std::condition_variable incoming_cv_;
    std::list<Information> incoming_;

    // If the network should be stopped
    std::mutex stop_lock_;
    bool stop_ = false;
};

#endif