#ifndef _INFORMATION_H_
#define _INFORMATION_H_

#include "Packet.h"
#include <cstddef>

class Information {
public:
    Information();
    Information(const Packet& packet, size_t peer_id);

    void setId(size_t peer_id);
    size_t getId() const;
    Packet& getPacket();

private:
    Packet packet_;
    size_t peer_id_;
};

#endif