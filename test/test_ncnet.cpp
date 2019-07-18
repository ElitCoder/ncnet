#include "Log.h"
#include "Server.h"

int main() {
    Server server;

    server.start(10000);
    server.stop();

    return 0;
}