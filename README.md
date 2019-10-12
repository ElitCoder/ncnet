# ncnet

Simple and effective C++ networking library currently using only TCP. Useful for quickly getting started with new projects and not spending hours fighting the Berkeley sockets.

## Features

* Quick setup with sane defaults
* Multithreaded
* Processing loops are provided
* Internal packet structure using C++11 operators <<, >>
* Encrypted network traffic

## Dependencies
* [cryptopp](https://github.com/weidai11/cryptopp)

## Install
#### Using the provided script

```console
$ git clone https://github.com/ElitCoder/ncnet.git
$ cd ncnet && ./build_install.sh
```

#### Using CMake

```console
$ git clone https://github.com/ElitCoder/ncnet.git
$ cd ncnet && mkdir -p build && cd build && cmake .. && make -j && sudo make install
```

## Platforms

* Linux
* Windows (upcoming)

## Example building with ncnet using GCC

```console
$ g++ -o ncnet_src ncnet_src.cpp -lncnet -lcryptopp
```

## Usage samples
#### Simple echo server
```c++
#include <ncnet/Server.h>

int main() {
    // Main server structure
    ncnet::Server server;

    // Bind and listen on all interfaces at the specified port
    server.start("", 10000);
    // Register callback function for received packets
    server.register_transfer_loop([&server] (auto &transfer) {
        // transfer is now populated with the incoming packet and the connection ID
        // transfer = [ packet, connection ID ]

        // Send echo back
        server.send_packet(transfer.get_packet(), transfer.get_connection_id());
    });

    // Main loop processing continues since packet handling is async
    //
    // ...
    //

    // Disconnect clients and cleanup
    server.stop();
}
```

#### Simple echo client
```c++
#include <ncnet/Client.h>

int main() {
    // Main client structure
    ncnet::Client client;

    // Connect to localhost:10000
    client.start("localhost", 10000);
    // Register callback function for received packets
    client.register_transfer_loop([&client] (auto &transfer) {
        // transfer is now populated with the incoming packet and the connection ID
        // transfer = [ packet, connection ID ]

        // Send echo back to server
        client.send_packet(transfer.get_packet(), transfer.get_connection_id());
    });

    // Create and send a echo packet
    ncnet::Packet echo;
    echo << "Hello, world!";
    client.send_packet(echo);

    // Main loop processing continues since packet handling is async
    //
    // ...
    //

    // Disconnect from server and cleanup
    client.stop();
}
```
