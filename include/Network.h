#pragma once

#include "Connection.h"
#include "EventPipe.h"
#include "Transfer.h"

#include <thread>
#include <vector>
#include <condition_variable>

namespace ncnet {
    class Network {
    public:
        static bool prepare_socket(int fd);
        virtual void send_packet(const Packet &packet, size_t peer_id = 0) final;
        BP_GET(socket, int)
        BP_GET(port, int)

        virtual bool start(const std::string &hostname, int port) = 0;
        virtual void stop() final; // Flush and shutdown
        Transfer get_packet(); // Wait and return when a packet is received

        // Starts internal loop
        void run();

    protected:
        std::thread network_; // Main network thread
        int socket_ = -1; // Main listening socket
        bool is_client_ = false;
        int port_ = -1;

        std::vector<Connection> connections_; // First connection is server in client case
        EventPipe pipe_; // Needed to interrupt when adding queued packets

    private:
        void sort_outgoing_packets(); // Moves outgoing packets to the correct connection queue
        // Selects sockets to listen on
        void select_connections(fd_set& read_set, fd_set& write_set, fd_set& error_set);
        // Read from connection
        bool read_data(Connection& connection);
        // Write to connection
        bool write_data(Connection& connection);

        std::mutex incoming_lock_;
        std::condition_variable incoming_cv_;
        std::list<Transfer> incoming_;
        std::mutex outgoing_lock_;
        std::list<Transfer> outgoing_; // Outgoing packet queue

        // If the network should be stopped
        std::mutex stop_lock_;
        bool stop_ = false;
    };
}