#include "Network.h"
#include "Server.h"
#include "Client.h"

using namespace std;

Server& Network::start_server(int port)
{
	servers_.emplace_back(make_shared<Server>());
	Server& server = *servers_.back();
	server.start(port);

	return server;
}

Client& Network::start_client(const string& hostname, int port)
{
	clients_.emplace_back(make_shared<Client>());
	Client& client = *clients_.back();
	client.start(hostname, port);

	return client;
}