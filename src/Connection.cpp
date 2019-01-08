#include "Connection.h"

#include <unistd.h>

using namespace std;

void Connection::setSocket(int fd) {
    socket_ = fd;

    // This is only done once, so set ID here
    static size_t id;
    id_ = id++;
}

int Connection::getSocket() const {
    return socket_;
}

void Connection::disconnect() {
    disconnected_ = true;

    close(socket_);
}

bool Connection::isConnected() const {
    return !disconnected_;
}

Packet& Connection::getPacketSkeleton() {
    if (incoming_.empty() || incoming_.back().isFinished()) {
        incoming_.emplace_back();
    }

    return incoming_.back();
}

size_t Connection::getId() const {
    return id_;
}