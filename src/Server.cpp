#include "Server.h"
#include "Log.h"

#include <unistd.h>
#include <cstring>
#include <netdb.h>

using namespace std;

static void networking(Server& server) {
    server.run();
}

bool Server::start(const string &hostname, int port) {
    // hostname is not used in server-mode
    (void)hostname;

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

        // Set re-usable
        int on = 1;
        if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) < 0) {
            Log(WARNING) << "Not reusable address\n";
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
        close(socket_);
        return false;
    }

    // Create networking thread and start processing
    network_ = thread(networking, ref(*this));
    port_ = port;

    return success;
}