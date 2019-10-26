#include "Network.h"
#include "Log.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <algorithm>
#include <cassert>
#include <unistd.h>
#include <cmath>

using namespace std;

namespace ncnet {
    static void run_internal_transfer_loop(Network &network, TransferFunction func) {
        // Loop until stopped
        while (true) {
            auto transfer = network.get_packet();
            if (transfer.get_is_exit()) {
                break;
            }
            // Call supplied function
            func(transfer);
        }
    }

    vector<string> Network::get_interface_ips() const {
        struct ifaddrs* interfaces;
        if (getifaddrs(&interfaces) == -1) {
            // Could not get interfaces
            Log(DEBUG) << "Could not retrieve IP interfaces";
            return vector<string>();
        }

        vector<string> ips;
        for (auto *iterator = interfaces; iterator; iterator = iterator->ifa_next) {
            if (!iterator->ifa_addr) {
                Log(DEBUG) << "Invalid IP address, ignoring";
                continue;
            }

            // Check for AF_INET family interfaces
            if (iterator->ifa_addr->sa_family == AF_INET) {
                auto ip = inet_ntoa(((struct sockaddr_in*)iterator->ifa_addr)->sin_addr);
                Log(DEBUG) << "Found IP " << ip;
                ips.push_back(ip);
            }
        }

        freeifaddrs(interfaces);
        return ips;
    }

    void Network::start_key_exchange(Connection &connection) {
        // Send opening packet containing generated public keys
        Packet packet;
        packet << connection.get_security().get_pub_dh_key();
        packet << connection.get_security().get_pub_sign_key();
        // Bypass send_packet to avoid encryption
        packet.finalize();
        connections_.front().add_outgoing_packet(packet);
    }

    bool Network::respond_key_exchange(Connection &connection, Transfer &transfer) {
        // Read supplied keys
        auto &packet = transfer.get_packet();
        string dh_pub;
        string sign_pub;
        packet >> dh_pub;
        packet >> sign_pub;

        // Encrypted CEK sent from server
        string encrypted_cek;

        if (!is_client_) {
            // Client -> server means respond with CEK
            try {
                encrypted_cek = connection.get_security().compute_shared_key(dh_pub, sign_pub);
            } catch (std::runtime_error &e) {
                // Disconnect client
                return false;
            }

            // Return our public DH key and public sign key along with CEK
            Packet key_response;
            key_response << connection.get_security().get_pub_dh_key() << connection.get_security().get_pub_sign_key() << encrypted_cek;
            // Bypass send_packet
            key_response.finalize();
            connection.add_outgoing_packet(key_response);
        } else {
            // Read encrypted CEK
            packet >> encrypted_cek;
            // Compute shared key and set new CEK
            connection.get_security().compute_shared_key(dh_pub, sign_pub);
            connection.get_security().set_encrypted_cek(encrypted_cek);
        }

        // All good
        return true;
    }

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
        auto &packet = connection.get_packet_skeleton();
        auto left = packet.left_in_packet();
        unsigned char *buffer;
        try {
            buffer = packet.get_writable_buffer(left);
        } catch (...) {
            // Bad packet size
            Log(WARN) << "Bad packet size detected, disconnecting client";
            return false;
        }

        auto received = recv(connection.get_socket(), buffer, left, 0);
        if (received <= 0) {
            if (received == -1 && errno == -EAGAIN) {
                return true; // Wait until next call
            }
            return false; // Error or disconnect
        }

        // Notify data was added
        packet.added_data(received);

        list<Transfer> incoming;
        // See if any packets are complete
        while (connection.has_incoming_packets()) {
            incoming.emplace_back(connection.get_id(), connection.get_incoming_packet());
        }

        if (connection.get_key_exchange()) {
            if (incoming.empty()) {
                // No full packet
                return true;
            }

            // When in secure transfer, the client should only send an auth packet and wait for server secret response
            if (incoming.size() > 1 || !respond_key_exchange(connection, incoming.front())) {
                // Disconnect
                Log(WARN) << "Disconnecting client due to invalid security protocol";
                return false;
            }

            // Everything OK, exit key exchange
            connection.set_key_exchange(false);
            incoming.pop_front();
        }

        // Decrypt incoming packets
        for (auto &transfer : incoming) {
            try {
                transfer.get_packet().decrypt(connection.get_security());
            } catch (runtime_error &e) {
                // Disconnect client
                Log(WARN) << "Decrypting failed, disconnecting client";
                return false;
            }
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
        auto sent = send(connection.get_socket(), packet.get_send_buffer(), packet.left_to_send(), 0);
        if (sent <= 0) {
            if (sent == -1 && errno == -EAGAIN) {
                return true; // Wait for next call
            }
            return false; // Error or disconnected
        }

        auto fully = packet.sent_data(sent);
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
                // If we're in key exchange, all packets should be held-off until key exchange is done
                if (connections_.front().get_key_exchange()) {
                    return;
                }

                for (auto &transfer : outgoing_) {
                    transfer.get_packet().encrypt(connections_.front().get_security());
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

                // Encrypt by default
                transfer.get_packet().encrypt(iterator->get_security());
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

            auto should_stop = false;
            {
                lock_guard<mutex> lock(stop_lock_);
                should_stop = stop_;
            }

            if (should_stop) {
                // Wake threads waiting for packets
                {
                    lock_guard<mutex> incoming_lock(incoming_lock_);
                    incoming_cv_.notify_all();
                }

                // Close all socket connections
                for (auto &connection : connections_) {
                    connection.disconnect();
                }

                // Close server socket
                if (!is_client_) {
                    close(get_socket());
                }

                // Gracefully exit
                Log(DEBUG) << "Exiting";
                return;
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
                    Log(DEBUG) << "Client connected (IP:" << ip << ")";

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

            // Check for disconnecting connections
            {
                lock_guard<mutex> lock(disconnect_lock_);
                for (auto &id : disconnect_connections_) {
                    auto iterator = find_if(connections_.begin(), connections_.end(), [&id] (auto &connection) {
                        return connection.get_id() == id;
                    });

                    if (iterator == connections_.end()) {
                        Log(WARN) << "Failed to find disconnecting client " << id;
                        continue;
                    }

                    // Disconnect
                    iterator->disconnect();
                }

                // Removed everything
                disconnect_connections_.clear();
            }

            // Iterate through connections
            for (auto &connection : connections_) {
                if (!connection.get_connected()) {
                    // Already gone
                    continue;
                }

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
            connections_.erase(remove_if(connections_.begin(), connections_.end(), [this] (auto& connection) {
                if (!connection.get_connected()) {
                    Log(DEBUG) << "Removing connection " << connection.get_id();
                    // Call disconnect callback if registered
                    if (disconnect_callback_ != nullptr) {
                        disconnect_callback_(connection.get_id());
                    }
                }
                return !connection.get_connected();
            }), connections_.end());

            // If we're in client mode, losing the connection is fatal
            if (is_client_ && connections_.empty()) {
                Log(ERROR) << "Lost connection to server!";
                // Simulate exit
                stop(false);
            }
        }
    }

    Transfer Network::get_packet() {
        bool should_stop = false;
        unique_lock<mutex> lock(incoming_lock_);
        incoming_cv_.wait(lock, [this, &should_stop] {
            lock_guard<mutex> stop_lock(stop_lock_);
            should_stop = stop_;
            return stop_ ? true : !incoming_.empty();
        });

        if (should_stop) {
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

    void Network::stop(bool wait) {
        {
            lock_guard<mutex> lock(stop_lock_);
            stop_ = true;

            // Wake pipe
            pipe_.activate();
        }

        // Avoid resource locking if we're simulating exit
        if (wait) {
            // Wait for exit
            network_.join();
        }

        {
            // Wait for transfer loops
            lock_guard<mutex> lock(transfer_loop_lock_);
            for (auto &transfer_thread : transfer_loops_) {
                transfer_thread.join();
            }
        }
    }

    void Network::send_packet(Packet &packet, size_t peer_id) {
        lock_guard<mutex> lock(outgoing_lock_);
        packet.finalize(); // Calculate headers if not done
        outgoing_.push_back(Transfer(peer_id, packet));

        Log(DEBUG) << "Pushing packet to peer " << peer_id;

        // Also wake up the pipe
        pipe_.activate();
    }

    void Network::disconnect(size_t id) {
        lock_guard<mutex> lock(disconnect_lock_);
        disconnect_connections_.push_back(id);

        // Wake pipe to process
        pipe_.activate();
    }

    void Network::register_transfer_loop(const TransferFunction &func) {
        lock_guard<mutex> lock(transfer_loop_lock_);
        // Start transfer thread and add to list to keep track
        transfer_loops_.emplace_back(thread(run_internal_transfer_loop, ref(*this), func));
    }

    void Network::run_transfer_loop(const TransferFunction &func) {
        // Blocking transfer loop
        run_internal_transfer_loop(ref(*this), func);
    }
}