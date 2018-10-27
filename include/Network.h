#ifndef NETWORK_H
#define NETWORK_H

#include <vector>
#include <string>
#include <memory>

class Server;
class Client;

class Network {
public:
	Server& start_server(int port);
	Client& start_client(const std::string& hostname, int port);

private:
	std::vector<std::shared_ptr<Server>> servers_;
	std::vector<std::shared_ptr<Client>> clients_;
};

#endif