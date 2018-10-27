#ifndef CLIENT_H
#define CLIENT_H

#include <string>

class Client {
public:
	bool start(const std::string& hostname, int port);
};

#endif