#include "Information.h"

Information::Information() {}

Information::Information(const Packet& packet, size_t peer_id) {
    packet_ = packet;
    peer_id_ = peer_id;
}

void Information::setId(size_t peer_id) {
    peer_id_ = peer_id;
}

size_t Information::getId() const {
    return peer_id_;
}

Packet& Information::getPacket() {
    return packet_;
}