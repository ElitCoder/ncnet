#include "Server.h"
#include "Log.h"

#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>

using namespace std;

static int processBuffer(Packet& packet, const unsigned char* buffer, int size) {
    size_t left = packet.getLeft(); // Returns left for header or left of packet

    if (left > (unsigned int)size) {
        left = size;
    }

    try {
        packet.addRaw(buffer, left);
    } catch (...) {
        Log(WARNING) << "Bad packet, throwing\n";
    }

    return left;
}

bool Server::read(Connection& connection) {
    constexpr auto BUFFER_SIZE = 1024 * 1024;
    static vector<unsigned char> buffer;

    if (buffer.empty()) {
        Log(DEBUG) << "Resizing static buffer\n";
        buffer.resize(BUFFER_SIZE);
    }

    int received = recv(connection.getSocket(), buffer.data(), BUFFER_SIZE, 0);
    if (received <= 0) {
        return false;
    }

    // Process buffer
    int processed = 0;
    while (processed < received) {
        processed += processBuffer(connection.getPacketSkeleton(), buffer.data(), received);
    }

    return true;
}

bool Server::write(Connection& connection) {
    return false;
}

void Server::prepareSets(fd_set& read_set, fd_set& write_set, fd_set& error_set) {
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_ZERO(&error_set);

    FD_SET(getSocket(), &read_set);
    FD_SET(getSocket(), &error_set);
}

void Server::run() {
    fd_set read_set;
    fd_set write_set;
    fd_set error_set;

    while (true) {
        prepareSets(read_set, write_set, error_set);

        if (!select(FD_SETSIZE, &read_set, &write_set, &error_set, NULL)) {
            Log(ERROR) << "Failed to select sockets\n";
            return;
        }

        if (FD_ISSET(getSocket(), &error_set)) {
            // Error
            Log(ERROR) << "Error in accepting socket\n";
            return;
        }

        if (FD_ISSET(getSocket(), &read_set)) {
            // Got connection
            struct sockaddr in_addr;
            socklen_t in_len = sizeof in_addr;
            int new_fd = accept(getSocket(), &in_addr, &in_len);
            if (new_fd == -1) {
                Log(WARNING) << "Failed to accept new connection\n";
                continue;
            }

            string hbuf(NI_MAXHOST, '\0');
            string sbuf(NI_MAXSERV, '\0');
            if (getnameinfo(&in_addr, in_len, (char*)hbuf.data(), hbuf.size(), (char*)sbuf.data(), sbuf.size(), NI_NUMERICHOST | NI_NUMERICSERV)) {
                Log(INFORMATION) << "Accepted connection on " << new_fd << "(host=" << hbuf << ", port=" << sbuf << ")" << "\n";
            } else {
                Log(WARNING) << "Failed to retrieve information about the connection\n";
            }

            prepareSocket(new_fd);
            Connection connection;
            connection.setSocket(new_fd);
            connections_.push_back(connection);
        }

        // Iterate through connections
        #pragma omp parallel for
        for (size_t i = 0; i < connections_.size(); i++) {
            auto& connection = connections_.at(i);

            if (FD_ISSET(connection.getSocket(), &error_set)) {
                Log(WARNING) << "Error on socket " << connection.getSocket() << endl;
                connection.disconnect();
            }

            if (FD_ISSET(connection.getSocket(), &read_set)) {
                // Read data from connection
                if (!read(connection)) {
                    connection.disconnect();
                }
            }

            if (FD_ISSET(connection.getSocket(), &write_set)) {
                // Write data to connection
                if (!write(connection)) {
                    connection.disconnect();
                }
            }
        }

        // Remove disconnected sockets
        connections_.erase(remove_if(connections_.begin(), connections_.end(), [] (auto& connection) {
            return !connection.isConnected();
        }), connections_.end());
    }
}

static void networking(Server& server) {
    server.run();
}

bool Server::start(int port) {
    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; // IPv4 + IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // All interfaces

    struct addrinfo* resulting_hints;
    int result = getaddrinfo(NULL, to_string(port).c_str(), &hints, &resulting_hints);
    if (result != 0) {
        Log(ERROR) << "getaddrinfo failed with code " << result << endl;
        return false;
    }

    bool success = false;
    for (struct addrinfo* i = resulting_hints; i != nullptr; i = i->ai_next) {
        socket_ = socket(i->ai_family, i->ai_socktype, i->ai_protocol);
        if (socket_ == -1) {
            continue; // Continue until success
        }

        result = bind(socket_, i->ai_addr, i->ai_addrlen);
        if (result == 0) {
            success = true;
            break; // Success
        }

        close(socket_);
    }

    freeaddrinfo(resulting_hints);

    if (!success) {
        Log(ERROR) << "Failed to bind to any interface on port " << port << endl;
        return success;
    }

    // Non-blocking mode
    prepareSocket(socket_);

    if (listen(socket_, SOMAXCONN) == -1) {
        Log(ERROR) << "Failed to listen to socket\n";
        return false;
    }

    // Create networking thread and start processing
    network_ = thread(networking, ref(*this));

    return success;
}

Information Server::get() {
}