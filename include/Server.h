#ifndef SERVER_H
#define SERVER_H

#include "Network.h"

class Server : public Network {
public:
    virtual bool start(const std::string &hostname, int port);
};

#endif