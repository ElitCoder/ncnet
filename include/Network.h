#ifndef NETWORK_H
#define NETWORK_H

#include <thread>

class Network {
public:
    //virtual bool connected() const;
    static bool prepareSocket(int fd);
    int getSocket() const;

protected:
    std::thread network_;
    int socket_;
};

#endif