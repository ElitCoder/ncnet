#include <ncnet/Server.h>
#include <ncnet/Client.h>

#include <iostream>
#include <atomic>
#include <cassert>

using namespace std;

bool verify_response(ncnet::Packet &packet) {
    cout << "Verifying response\n";
    string tmp;
    packet >> tmp;
    assert(tmp == "string");
    packet >> tmp;
    assert(tmp == "long string");
    bool bool_val;
    packet >> bool_val;
    assert(bool_val);
    short short_val;
    packet >> short_val;
    assert(short_val == 1);
    int int_val;
    packet >> int_val;
    assert(int_val == 20000000);
    long long int long_val;
    packet >> long_val;
    assert(long_val == 2000000000000);
    packet >> int_val;
    assert(int_val == -10000);
    packet >> long_val;
    assert(long_val == -1000000000);
    packet >> tmp;
    assert(tmp == "both");
    cout << "[PASSED]\n";
    return false;
}

ncnet::Packet create_test_packet() {
    ncnet::Packet packet;
    packet << "string";
    packet << "long string";
    packet << true;
    packet << 1;
    packet << 20000000;
    packet << 2000000000000;
    packet << -10000;
    packet << -1000000000 << "both";

    return packet;
}

int main() {
    ncnet::Server server;
    auto port = 15500;
    server.start("", port);
    server.register_transfer_loop([&server] (auto &transfer) {
        // Echo
        server.send_packet(transfer.get_packet(), transfer.get_connection_id());
        cout << "Echoing\n";
    });

    ncnet::Client client;
    client.start("localhost", port);
    atomic<bool> quit;
    atomic<bool> failed;
    quit.store(false);
    failed.store(true);
    client.register_transfer_loop([&client, &quit, &failed] (auto &transfer) {
        failed.store(verify_response(transfer.get_packet()));
        quit.store(true);
    });

    auto test_packet = create_test_packet();
    client.send_packet(test_packet);
    cout << "Sent test packet\n";

    // Wait
    while (true) {
        if (quit.load()) {
            break;
        }

        using namespace literals::chrono_literals;
        this_thread::sleep_for(1ms);
    }

    client.stop();
    server.stop();
    return failed.load() ? 0 : -1;
}