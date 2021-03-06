#pragma once

#include "Boilerplate.h"
#include "Packet.h"
#include "Security.h"

#include <list>

namespace ncnet {
    class Connection {
    public:
        explicit Connection(); // Force new connection IDs
        BP_SET_GET(socket, int)
        BP_SET_GET(key_exchange, bool)
        BP_GET(id, size_t)
        BP_GET(connected, bool)
        BP_GET(security, Security &)

        // Status
        void disconnect();
        bool has_incoming_packets() const;
        bool has_outgoing_packets() const;

        // Packet modifiers
        Packet get_incoming_packet(); // Pops incoming packet if any
        Packet& get_outgoing_packet(); // Returns next packet to send
        void pop_outgoing(); // Remove first packet when done sending
        void add_outgoing_packet(const Packet& packet); // Add packet to send
        Packet& get_packet_skeleton(); // Get skeleton to insert data to

    private:
        int socket_ = -1;
        bool connected_ = true;
        size_t id_ = 0;

        std::list<Packet> incoming_;
        std::list<Packet> outgoing_;

        // Secure transfer
        bool key_exchange_ = true;
        Security security_;
    };
}