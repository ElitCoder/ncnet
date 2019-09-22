#include "Connection.h"
#include "Log.h"

#include <unistd.h>
#include <cassert>

using namespace std;

namespace ncnet {
    Connection::Connection() {
        static size_t id;
        id_ = ++id;
    }

    void Connection::disconnect() {
        connected_ = false;
        close(socket_);
    }

    Packet& Connection::get_packet_skeleton() {
        if (incoming_.empty() || incoming_.back().has_received_full_packet()) {
            incoming_.emplace_back();
        }

        return incoming_.back();
    }

    bool Connection::has_incoming_packets() const {
        return incoming_.empty() ? false : incoming_.front().has_received_full_packet();
    }

    Packet Connection::get_incoming_packet() {
        assert(has_incoming_packets());

        auto packet = incoming_.front();
        incoming_.pop_front();

        return packet;
    }

    bool Connection::has_outgoing_packets() const {
        return !outgoing_.empty();
    }

    Packet& Connection::get_outgoing_packet() {
        assert(!outgoing_.empty());
        return outgoing_.front();
    }

    void Connection::pop_outgoing() {
        outgoing_.pop_front();
    }

    void Connection::add_outgoing_packet(const Packet& packet) {
        outgoing_.push_back(packet);
    }
}