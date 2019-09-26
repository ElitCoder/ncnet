#include <ncnet/Server.h>
#include <ncnet/Client.h>
#include <iostream>

using namespace std;

void run_server(ncnet::Server &server) {
    server.start("", 12000);
    while (true) {
        auto packet = server.get_packet();
        if (packet.get_is_exit()) {
            break;
        }
        // Echo
        server.send_packet(packet.get_packet(), packet.get_connection_id());
    }
}

void run_client() {
    string TEST_STRING = "hej server 123";
    ncnet::Client client;
    client.start("localhost", 12000);
    ncnet::Packet packet;
    packet << TEST_STRING;
    packet << 123;
    packet << 8000000000;
    client.send_packet(packet);
    auto transfer = client.get_packet();
    string tmp;
    int int_tmp;
    size_t size_tmp;
    transfer.get_packet() >> tmp;
    transfer.get_packet() >> int_tmp;
    transfer.get_packet() >> size_tmp;
    cout << "Server responded with (" << tmp << ") " << int_tmp << " " << size_tmp << "\n";
    client.stop();
}

int main() {
    ncnet::Server server;
    auto server_thread = thread(run_server, ref(server));
    auto client_thread = thread(run_client);
    client_thread.join();
    server.stop();
    server_thread.join();
}