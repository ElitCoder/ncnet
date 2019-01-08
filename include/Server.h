#ifndef SERVER_H
#define SERVER_H

#include "Network.h"
#include "Information.h"
#include "Connection.h"

#include <vector>

class Server : public Network {
public:
    bool start(int port);
    void run();

    Information get();

private:
    void prepareSets(fd_set& read_set, fd_set& write_set, fd_set& error_set);
    bool read(Connection& connection);
    bool write(Connection& connection);

    std::vector<Connection> connections_;
};

#endif