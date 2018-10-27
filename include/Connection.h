#ifndef CONNECTION_H
#define CONNECTION_H

#include "PartialPacket.h"

#include <list>
#include <mutex>
#include <memory>

class Connection {
public:
    explicit Connection(int socket);

    bool operator==(const Connection &connection);

    PartialPacket& get_partial_packet();
    void add_partial_packet(const PartialPacket &partial);
    bool has_incoming_packet() const;
    PartialPacket& get_incoming_packet();
    void processed_packet();

    size_t packets_waiting();
    void reduce_wait();
    void increase_wait();

    bool is_verified() const;
    int get_socket() const;
    size_t get_id() const;
    void set_ip(const std::string& ip);
    const std::string& get_ip() const;

private:
    std::list<PartialPacket> in_queue_;

    size_t waiting_processing_ = 0;
    std::shared_ptr<std::mutex> waiting_processing_mutex_;

    size_t id_ = 0;
    std::string ip_;
    int socket_;
};

#endif