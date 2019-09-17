#include "Connection.h"
#include "Log.h"

#include <unistd.h>

using namespace std;

namespace ncnet {
    void Connection::setSocket(int fd) {
        socket_ = fd;

        // This is only done once, so set ID here
        static size_t id;
        id_ = ++id;
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

    bool Connection::hasIncoming() const {
        return incoming_.empty() ? false : incoming_.front().isFinished();
    }

    Packet Connection::getIncoming() {
        if (!hasIncoming()) {
            Log(WARN) << "Trying to retrieve incoming packets when there are none";
        }

        auto packet = incoming_.front();
        incoming_.pop_front();

        return packet;
    }

    bool Connection::hasOutgoing() const {
        return !outgoing_.empty();
    }

    Packet& Connection::getOutgoing() {
        return outgoing_.front();
    }

    void Connection::doneOutgoing() {
        outgoing_.pop_front();
    }

    void Connection::addOutgoing(Packet& packet) {
        outgoing_.push_back(packet);
    }
}