#include <ncnet/Server.h>
#include <ncnet/Client.h>
#include <ncnet/Log.h>

#include <iostream>
#include <atomic>
#include <cassert>

using namespace std;
using namespace ncnet;

bool verify_response(ncnet::Packet &packet) {
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
    auto byte = packet.read_byte();
    assert(byte == 2);
    packet >> tmp;
    assert(tmp == "after");
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
    packet.add_byte(2);
    packet << "after";

    return packet;
}

int main() {
    ncnet::Log::enable(true);

    ncnet::Server server;
    auto port = 15500;
    server.start("", port);
    server.register_transfer_loop([&server] (auto &transfer) {
        // Echo
        server.send_packet(transfer.get_packet(), transfer.get_connection_id());
    });

    ncnet::Client client;
    client.start("localhost", port);
    mutex lock;
    auto quit = false;
    auto failed = true;
    client.register_transfer_loop([&client, &quit, &failed, &lock] (auto &transfer) {
        lock_guard<mutex> failed_lock(lock);
        failed = verify_response(transfer.get_packet());
        quit = true;
    });

    auto test_packet = create_test_packet();
    client.send_packet(test_packet);

    // Wait
    while (true) {
        {
            lock_guard<mutex> loop_lock(lock);
            if (quit) {
                break;
            }
        }

        using namespace literals::chrono_literals;
        this_thread::sleep_for(1ms);
    }

    client.stop();
    server.stop();
    return failed ? 0 : -1;
}