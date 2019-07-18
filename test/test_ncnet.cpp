#include "Log.h"
#include "Server.h"

int main() {
    Server server;

    server.start(10000);

    while (true) {
        server.get();
    }

    return 0;
}