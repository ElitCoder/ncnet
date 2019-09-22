#include "Network.h"
#include "Log.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <algorithm>
#include <cassert>
#include <unistd.h>

using namespace std;

namespace ncnet {
    static int process_buffer(Packet& packet, const unsigned char* buffer, int size) {
        size_t left = packet.getLeft(); // Returns left for header or left of packet

        if (left > (unsigned int)size) {
            left = size;
        }

        try {
            packet.addRaw(buffer, left);
        } catch (...) {
            Log(WARN) << "Bad packet, throwing";
        }

        return left;
    }

    //
    // API
    //

    bool Network::prepare_socket(int fd) {
        // Just set non-blocking for now
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) {
            Log(ERROR) << "Failed to get flags from socket";
            return false;
        }

        flags |= O_NONBLOCK;

        if (fcntl(fd, F_SETFL, flags) == -1) {
            Log(WARN) << "Failed to set non-blocking socket";
        }

        int on = 1;
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&on), sizeof(on)) < 0) {
            Log(WARN) << "Failed to set TCP_NODELAY";
        }

        return true;
    }

    bool Network::read_data(Connection& connection) {
        constexpr auto BUFFER_SIZE = 1024 * 1024;
        vector<unsigned char> buffer;
        buffer.resize(BUFFER_SIZE);

        int received = recv(connection.get_socket(), buffer.data(), BUFFER_SIZE, 0);
        if (received <= 0) {
            if (received == -1 && errno == -EAGAIN) {
                return true; // Wait until next call
            }
            return false; // Error or disconnected
        }

        // Process buffer
        int processed = 0;
        while (processed < received) {
            auto i = process_buffer(connection.get_packet_skeleton(), buffer.data() + processed, received - processed);
            processed += i;

            // TODO: Can this even happen?
            if (i == 0) {
                Log(ERROR) << "Fatal assert error, not processing data";
                assert(true);
            }
        }

        list<Transfer> incoming;
        // See if any packets are complete
        while (connection.has_incoming_packets()) {
            incoming.emplace_back(connection.get_id(), connection.get_incoming_packet());
        }

        if (!incoming.empty()) {
            // Add to process queue
            lock_guard<mutex> lock(incoming_lock_);
            incoming_.insert(incoming_.end(), incoming.begin(), incoming.end());

            if (incoming_.size() > 1) {
                incoming_cv_.notify_all();
            } else {
                incoming_cv_.notify_one();
            }
        }

        return true;
    }

    bool Network::write_data(Connection& connection) {
        auto& packet = connection.get_outgoing_packet();

        Log(DEBUG) << "Writing data to " << connection.get_id();

        int sent = send(connection.get_socket(), packet.getData() + packet.getSent(), packet.getSize() - packet.getSent(), 0);
        if (sent <= 0) {
            if (sent == -1 && errno == -EAGAIN) {
                return true; // Wait for next call
            }
            return false; // Error or disconnected
        }

        auto fully = packet.addSent(sent);
        if (fully) {
            // Remove packet, it's fully sent
            connection.pop_outgoing();
        }

        // Success
        return true;
    }

    void Network::select_connections(fd_set& read_set, fd_set& write_set, fd_set& error_set) {
        FD_ZERO(&read_set);
        FD_ZERO(&write_set);
        FD_ZERO(&error_set);

        // Ignore main socket in client mode since server is the only connection
        if (!is_client_) {
            FD_SET(get_socket(), &read_set);
            FD_SET(get_socket(), &error_set);
        }

        FD_SET(pipe_.get_socket(), &read_set);
        FD_SET(pipe_.get_socket(), &error_set);

        for (auto& connection : connections_) {
            FD_SET(connection.get_socket(), &read_set);
            FD_SET(connection.get_socket(), &error_set);

            if (connection.has_outgoing_packets()) {
                FD_SET(connection.get_socket(), &write_set);
            }
        }
    }

    void Network::sort_outgoing_packets() {
        lock_guard<mutex> lock(outgoing_lock_);

        // When we're in client-mode, all packets should go to the server
        if (is_client_) {
            if (connections_.empty()) {
                // This case already handled by the main loop
            } else {
                for (auto &transfer : outgoing_) {
                    connections_.front().add_outgoing_packet(transfer.get_packet());
                }
            }
        } else {
            // FIXME: This is probably time-consuming
            for (auto& transfer : outgoing_) {
                // Find right connection
                auto iterator = find_if(connections_.begin(), connections_.end(), [&transfer] (auto& connection) {
                    return connection.get_id() == transfer.get_connection_id();
                });

                if (iterator == connections_.end()) {
                    Log(DEBUG) << "Did not find connection with ID " << transfer.get_connection_id();
                    // Not found, ignore
                    continue;
                }

                iterator->add_outgoing_packet(transfer.get_packet());
            }
        }

        // Clean outgoing packets since they are put in queue or removed
        outgoing_.clear();
    }

    void Network::run() {
        fd_set read_set;
        fd_set write_set;
        fd_set error_set;

        while (true) {
            sort_outgoing_packets();
            select_connections(read_set, write_set, error_set);

            if (!select(FD_SETSIZE, &read_set, &write_set, &error_set, NULL)) {
                Log(ERROR) << "Failed to select sockets";
                return;
            }

            {
                lock_guard<mutex> lock(stop_lock_);
                if (stop_) {
                    // Wake threads waiting for packets
                    incoming_cv_.notify_all();

                    // Close all socket connections
                    for_each(connections_.begin(), connections_.end(), [] (auto &connection) {
                        connection.disconnect();
                    });

                    // Close server socket
                    close(get_socket());

                    // Gracefully exit
                    Log(DEBUG) << "Exiting";
                    return;
                }
            }

            // Only check accepting socket in server-mode
            if (!is_client_) {
                if (FD_ISSET(get_socket(), &error_set)) {
                    // Error
                    Log(ERROR) << "Error in accepting socket";
                    return;
                }

                if (FD_ISSET(get_socket(), &read_set)) {
                    // Got connection
                    struct sockaddr in_addr;
                    socklen_t in_len = sizeof in_addr;
                    int new_fd = accept(get_socket(), &in_addr, &in_len);
                    if (new_fd == -1) {
                        Log(WARN) << "Failed to accept new connection";
                        continue;
                    }

                    auto ip = inet_ntoa(((sockaddr_in*)&in_addr)->sin_addr);
                    Log(INFO) << "Client connected (IP:" << ip << ")";

                    prepare_socket(new_fd);
                    Connection connection;
                    connection.set_socket(new_fd);
                    connections_.push_back(connection);
                }
            }

            // Check pipe
            if (FD_ISSET(pipe_.get_socket(), &read_set)) {
                Log(DEBUG) << "Activating pipe";
                pipe_.reset();
            }

            if (FD_ISSET(pipe_.get_socket(), &error_set)) {
                Log(ERROR) << "Got pipe error";
                return;
            }

            // Iterate through connections
            #pragma omp parallel for
            for (size_t i = 0; i < connections_.size(); i++) {
                auto& connection = connections_.at(i);

                if (FD_ISSET(connection.get_socket(), &error_set)) {
                    Log(WARN) << "Error on socket " << connection.get_socket();
                    connection.disconnect();
                }

                if (FD_ISSET(connection.get_socket(), &read_set)) {
                    // Read data from connection
                    if (!read_data(connection)) {
                        connection.disconnect();
                    }
                }

                if (FD_ISSET(connection.get_socket(), &write_set)) {
                    // Write data to connection
                    if (!write_data(connection)) {
                        connection.disconnect();
                    }
                }
            }

            // Remove disconnected sockets
            connections_.erase(remove_if(connections_.begin(), connections_.end(), [] (auto& connection) {
                if (!connection.is_connected()) {
                    Log(DEBUG) << "Removing connection " << connection.get_id();
                    // TODO: Call disconnect callback here
                }
                return !connection.is_connected();
            }), connections_.end());

            // If we're in client mode, losing the connection is fatal
            if (is_client_ && connections_.empty()) {
                Log(ERROR) << "Lost connection to server!";
                // Simulate exit
                stop();
            }
        }
    }

    Transfer Network::get_packet() {
        unique_lock<mutex> lock(incoming_lock_);
        incoming_cv_.wait(lock, [this] {
            lock_guard<mutex> stop_lock(stop_lock_);
            return stop_ ? true : !incoming_.empty();
        });

        lock_guard<mutex> stop_lock(stop_lock_);
        if (stop_) {
            // Exiting
            Transfer transfer;
            transfer.set_is_exit(true); // Signal exit packet
            return transfer;
        }

        auto transfer = incoming_.front();
        incoming_.pop_front();

        Log(DEBUG) << "Returning packet to peer " << transfer.get_connection_id();
        return transfer;
    }

    void Network::stop() {
        {
            lock_guard<mutex> lock(stop_lock_);
            stop_ = true;

            // Wake pipe
            pipe_.activate();
        }

        // Wait for exit
        network_.join();
    }

    void Network::send_packet(const Packet &packet, size_t peer_id) {
        lock_guard<mutex> lock(outgoing_lock_);
        outgoing_.push_back(Transfer(peer_id, packet));

        Log(DEBUG) << "Pushing packet to peer " << peer_id;

        // Also wake up the pipe
        pipe_.activate();
    }
}