#ifndef SERVER_H
#define SERVER_H

#include "Network.h"
#include "Information.h"

class Server : public Network {
public:
    bool start(int port);

    Information get();
};

#endif