#ifndef CLIENT_H
#define CLIENT_H

#include "Network.h"
//#include "Packet.h"

class Client : public Network {
public:
    bool start(const std::string& hostname, int port);

	//Packet get();
};

#endif