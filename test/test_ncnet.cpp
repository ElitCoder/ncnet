#include "Log.h"
#include "Server.h"
#include "Client.h"

using namespace std;

void run_server() {
    Server server;
    server.start("", 10000);
    while (true) {
        server.get();
    }
}

void run_client() {
    Client client;
    client.start("localhost", 10000);
    client.stop();
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        run_server();
    } else {
        if (string(argv[1]) == "server") {
            run_server();
        } else if (string(argv[1]) == "client") {
            run_client();
        }
    }

    return 0;
}