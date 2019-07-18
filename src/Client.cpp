#include "Client.h"
#include "Log.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

using namespace std;

static void networking(Client& client) {
    client.run();
}

bool Client::start(const string &hostname, int port) {
    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ < 0) {
        Log(ERROR) << "socket() failed\n";
        return false;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;

    hostent *hp = gethostbyname(hostname.c_str());
    if (!hp) {
        Log(ERROR) << "Could not find host " << hostname << endl;
        close(socket_);
        return false;
    }

    memcpy(reinterpret_cast<char*>(&address.sin_addr), reinterpret_cast<char*>(hp->h_addr), hp->h_length);
    address.sin_port = htons(port);

    if (connect(socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        Log(ERROR) << "Could not connect to " << hostname << ":" << port << endl;
        close(socket_);
        return false;
    }

    // Set non-blocking and TCP_NODELAY
    prepareSocket(socket_);

    // Mark as client-mode
    is_client_ = true;

    // Add server as only connection
    Connection connection;
    connection.setSocket(socket_);
    connections_.push_back(connection);

    Log(INFORMATION) << "Connected to " << hostname << ":" << port << endl;

    // Create networking thread and start processing
    network_ = thread(networking, ref(*this));
    port_ = port;

    return true;
}

bool Client::send(const Packet &packet) {
    lock_guard<mutex> lock(outgoing_lock_);
    outgoing_.push_back(Information(packet, 0));

    // Also wake up the pipe
    pipe_.setPipe();

    // In lack of error messages
    return true;
}