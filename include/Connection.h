#ifndef CONNECTION_H
#define CONNECTION_H

#include <list>
#include <mutex>
#include <memory>

class Packet;
class PartialPacket;

class Connection {
public:
    explicit Connection(const int socket);

    bool operator==(const Connection &connection);
    bool operator==(const int fd);

    int getSocket() const;
    PartialPacket& getPartialPacket();
    void addPartialPacket(const PartialPacket &partialPacket);
    bool hasIncomingPacket() const;
    PartialPacket& getIncomingPacket();
    void processedPacket();

    bool isVerified() const;
    void setIP(const std::string& ip);

    size_t packetsWaiting();
    void reducePacketsWaiting();
    void increasePacketsWaiting();

    size_t getUniqueID() const;
    const std::string& getIP() const;

private:
    int socket_;
    std::list<PartialPacket> in_queue_;

    size_t waiting_processing_ = 0;
    std::shared_ptr<std::mutex> waiting_processing_mutex_;

    size_t unique_id_ = 0;
    std::string ip_;
};

#endif