#include "Information.h"

Information::Information(const Packet& packet, size_t peer_id) {
    packet_ = packet;
    peer_id_ = peer_id;
}

size_t Information::getId() const {
    return peer_id_;
}

Packet& Information::getPacket() {
    return packet_;
}