#ifndef CLIENT_H
#define CLIENT_H

#include "Network.h"

class Client : public Network {
public:
    virtual bool start(const std::string& hostname, int port);
};

#endif