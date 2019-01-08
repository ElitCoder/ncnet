#include "Network.h"
#include "Log.h"
#include <fcntl.h>

bool Network::prepareSocket(int fd) {
    // Just set non-blocking for now
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        Log(WARNING) << "Failed to get flags from fd\n";
        return false;
    }

    flags |= O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) == -1) {
        Log(WARNING) << "Failed to set non-blocking socket\n";
        return false;
    }

    return true;
}

int Network::getSocket() const {
    return socket_;
}