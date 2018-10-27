#include "Connection.h"
#include "Log.h"
#include "PartialPacket.h"

#include <fcntl.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

using namespace std;

Connection::Connection(int socket)
{
    socket_ = socket;
    waiting_processing_mutex_ = make_shared<mutex>();

    if (fcntl(socket_, F_SETFL, O_NONBLOCK) == -1)
        Log(WARNING) << "Could not make non-blocking sockets\n";

    int on = 1;

    if (setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY,
            reinterpret_cast<char*>(&on), sizeof on) < 0) {
        Log(WARNING) << "Could not set TCP_NODELAY\n";
    }

    static size_t id;
    id_ = id++;
}

bool Connection::operator==(const Connection &connection)
{
    return id_ == connection.id_;
}

size_t Connection::get_id() const
{
    return id_;
}

int Connection::get_socket() const
{
    return socket_;
}

PartialPacket& Connection::get_partial_packet()
{
    if (in_queue_.empty() || in_queue_.back().isFinished()) {
        add_partial_packet(PartialPacket());
    }

    return in_queue_.back();
}

void Connection::add_partial_packet(const PartialPacket &partial)
{
    in_queue_.push_back(partial);
}

bool Connection::has_incoming_packet() const
{
    return in_queue_.empty() ? false : in_queue_.front().isFinished();
}

PartialPacket& Connection::get_incoming_packet()
{
    if (in_queue_.empty()) {
        Log(ERROR) << "Trying to get an incoming packet while the inQueue " \
        "is empty\n";
    }

    return in_queue_.front();
}

void Connection::processed_packet()
{
    if (in_queue_.empty()) {
        Log(ERROR) << "Trying to pop_front when the inQueue is empty\n";

        return;
    }

    in_queue_.pop_front();
    increase_wait();
}

// TODO: Implement this on a later stage
bool Connection::is_verified() const
{
    return true;
}

size_t Connection::packets_waiting()
{
    lock_guard<mutex> lock(*waiting_processing_mutex_);

    return waiting_processing_;
}

void Connection::reduce_wait()
{
    lock_guard<mutex> lock(*waiting_processing_mutex_);

    if (waiting_processing_ > 0)
        waiting_processing_--;
}

void Connection::increase_wait()
{
    lock_guard<mutex> lock(*waiting_processing_mutex_);

    waiting_processing_++;
}

void Connection::set_ip(const string& ip)
{
    ip_ = ip;
}

const string& Connection::get_ip() const
{
    return ip_;
}