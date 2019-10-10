#include "Client.h"
#include "Log.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

using namespace std;

namespace ncnet {
    static void networking(Client& client) {
        client.run();
    }

    bool Client::start(const string &hostname, int port) {
        socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socket_ < 0) {
            Log(ERROR) << "Failed to create main socket";
            return false;
        }

        sockaddr_in address;
        address.sin_family = AF_INET;

        hostent *hp = gethostbyname(hostname.c_str());
        if (!hp) {
            Log(ERROR) << "Could not resolve DNS hostname " << hostname;
            close(socket_);
            return false;
        }

        memcpy(reinterpret_cast<char*>(&address.sin_addr), reinterpret_cast<char*>(hp->h_addr), hp->h_length);
        address.sin_port = htons(port);

        if (connect(socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
            Log(WARN) << "Could not connect to " << hostname << ":" << port;
            close(socket_);
            return false;
        }

        // Set non-blocking and TCP_NODELAY
        prepare_socket(socket_);

        // Mark as client-mode
        is_client_ = true;

        // Add server as only connection
        Connection connection;
        connection.set_socket(socket_);
        connections_.push_back(connection);

        Log(DEBUG) << "Connected to " << hostname << ":" << port;

        // Create networking thread and start processing
        network_ = thread(networking, ref(*this));
        port_ = port;

        return true;
    }
}