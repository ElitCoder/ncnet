#pragma once

#include "Network.h"

namespace ncnet {
    class Server : public Network {
    public:
        virtual bool start(const std::string &hostname, int port);
    };
}