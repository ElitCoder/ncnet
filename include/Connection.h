#ifndef CONNECTION_H
#define CONNECTION_H

#include "Packet.h"

#include <list>

class Connection {
public:
    void setSocket(int fd);
    int getSocket() const;
    void disconnect();
    bool isConnected() const;
    Packet& getPacketSkeleton();
    size_t getId() const;

private:
    int socket_ = -1;
    bool disconnected_ = false;
    size_t id_ = 0;

    std::list<Packet> incoming_;
};

#endif