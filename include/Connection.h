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
    bool hasIncoming() const;
    bool hasOutgoing() const;
    Packet getIncoming(); // Pops
    Packet& getOutgoing(); // Non-modifier
    void doneOutgoing();
    void addOutgoing(Packet& packet);
    Packet& getPacketSkeleton();
    size_t getId() const;

private:
    int socket_ = -1;
    bool disconnected_ = false;
    size_t id_ = 0;

    std::list<Packet> incoming_;
    std::list<Packet> outgoing_;
};

#endif