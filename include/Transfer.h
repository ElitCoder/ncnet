#pragma once

#include "Packet.h"
#include "Boilerplate.h"

#include <cstddef>

namespace ncnet {
    // Transfer composes a struct of a connection ID and a Packet.
    class Transfer {
    public:
        Transfer() {}
        Transfer(size_t connection_id, const Packet &packet) : connection_id_(connection_id), packet_(packet) {}
        BP_SET_GET(connection_id, size_t)
        BP_SET_GET(packet, Packet &)
        BP_SET_GET(is_exit, bool)

    private:
        size_t connection_id_ = 0;
        Packet packet_;
        bool is_exit_ = false;
    };
}