#include "Server.h"
#include "Log.h"
#include "CommonNetwork.h"
#include "EventPipe.h"
#include "Packet.h"
#include "Connection.h"
#include "PartialPacket.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <algorithm>

using namespace std;

// Thread functions
void accept_thread(Server& server);
static void stats_thread(Server& server);
static void recv_thread(Server& server, int id);
static void send_thread(Server& server, int id);

Server::~Server()
{
	// Disconnect Clients
    for (auto& queue : connections_) {
        for (auto& connection : queue)
            close(connection.second.get_socket());
    }
}

bool Server::start(int port)
{
	if (!setup(port)) {
		return false;
	}

	for (int i = 0; i < num_send_threads_; i++) {
        outgoing_cv_.push_back(make_shared<condition_variable>());
        outgoing_mutex_.push_back(make_shared<mutex>());
        outgoing_packets_.emplace_back();
    }

    pipes_.resize(num_recv_threads_);
    connections_.resize(num_recv_threads_);

    for (int i = 0; i < num_recv_threads_; i++)
        connections_mutex_.push_back(make_shared<mutex>());

    accept_thread_ = thread(accept_thread, ref(*this));
    stats_thread_ = thread(stats_thread, ref(*this));

    send_threads_.resize(num_send_threads_);

    for (int i = 0; i < num_send_threads_; i++)
        send_threads_.at(i) = thread(send_thread, ref(*this), i);

    recv_threads_.resize(num_recv_threads_);

    for (int i = 0; i < num_recv_threads_; i++) {
		recv_threads_.at(i) = thread(recv_thread, ref(*this), i);
	}

	return true;
}

bool Server::setup(int port)
{
    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ < 0) {
        Log(ERROR) << "socket() failed\n";
        return false;
    }

    int on = 1;
    if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR,
			reinterpret_cast<char*>(&on), sizeof on) < 0) {
        Log(ERROR) << "Not reusable address\n";
        return false;
    }

    sockaddr_in information;
    information.sin_family = AF_INET;
    information.sin_addr.s_addr = INADDR_ANY;
    information.sin_port = htons(port);

    if (bind(socket_, reinterpret_cast<sockaddr*>(&information),
			sizeof information) < 0) {
        Log(ERROR) << "bind() failed\n";
        close(socket_);
        return false;
    }

    size_t information_size = sizeof information;

	if (getsockname(socket_, reinterpret_cast<sockaddr*>(&information),
			reinterpret_cast<socklen_t*>(&information_size)) < 0) {
        Log(ERROR) << "Could not get address information\n";
        close(socket_);
        return false;
	}

	listen(socket_, 16);

    return true;
}

void accept_thread(Server& server)
{
	fd_set read_set, error_set;

    while (true) {
        server.set_fd_accept(read_set, error_set);

        if (!server.select_accept(read_set, error_set)) {
            break;
        }
    }

    Log(ERROR) << "Terminating acceptThread\n";
}

static void stats_thread(Server& server)
{
	// TODO: Implement stats
	(void)server;
}

static void send_thread(Server& server, int id)
{
	fd_set send_set, error_set;

    while (true) {
        // Wait for something to send
        server.wait_for_outgoing_packets(id);

        unordered_set<int> fds;
        server.set_fd_send(send_set, error_set, id, fds);

        if (!server.select_send(send_set, error_set, id, fds)) {
            break;
        }
    }

    Log(ERROR) << "Terminating sendThread\n";
}

static void recv_thread(Server& server, int id)
{
	fd_set read_set, error_set;
    array<unsigned char, BUFFER_SIZE> buffer;

    while (true) {
        server.set_fd_recv(read_set, error_set, id);

        if (!server.select_recv(read_set, error_set, buffer.data(), id)) {
            break;
        }
    }

    Log(ERROR) << "Terminating receiveThread\n";
}

void Server::set_fd_accept(fd_set &read_set, fd_set &error_set)
{
    FD_ZERO(&read_set);
    FD_ZERO(&error_set);

    FD_SET(socket_, &read_set);
    FD_SET(socket_, &error_set);
}

void Server::set_fd_recv(fd_set &read_set, fd_set &error_set, int id)
{
    FD_ZERO(&read_set);
    FD_ZERO(&error_set);

    FD_SET(pipes_.at(id).get_socket(), &read_set);
    FD_SET(pipes_.at(id).get_socket(), &error_set);

	Log(NONE) << "C " << connections_mutex_.size() << " " << id << endl;

    lock_guard<mutex> guard(*connections_mutex_.at(id));

    for_each(connections_.at(id).begin(), connections_.at(id).end(),
			[&read_set, &error_set] (auto& connection) {
        FD_SET(connection.second.get_socket(), &read_set);
        FD_SET(connection.second.get_socket(), &error_set);
    });
}

void Server::set_fd_send(fd_set& send_set, fd_set& error_set, int id,
		unordered_set<int>& fds)
{
    FD_ZERO(&send_set);
    FD_ZERO(&error_set);

    lock_guard<mutex> lock(*outgoing_mutex_.at(id));

    // Remove duplicate file descriptors
    for (auto& peer : outgoing_packets_.at(id))
        fds.insert(peer.first);

    // Insert into fd_set's
    for (auto& fd : fds) {
        FD_SET(fd, &send_set);
        FD_SET(fd, &error_set);
    }
}

bool Server::select_recv(fd_set &read_set, fd_set &error_set,
		unsigned char *buffer, int id)
{
    if (!select(FD_SETSIZE, &read_set, NULL, &error_set, NULL)) {
        Log(ERROR) << "select() returned 0 when there's no timeout on " \
		"receiving thread " << id << endl;
        return false;
    }

    if(FD_ISSET(pipes_.at(id).get_socket(), &error_set)) {
        Log(ERROR) << "errorSet was set in pipe, errno = " << errno << '\n';
        return false;
    }

    if(FD_ISSET(pipes_.at(id).get_socket(), &read_set)) {
        pipes_.at(id).reset();
        return true;
    }

    lock_guard<mutex> guard(*connections_mutex_.at(id));

    for(auto& connection_peer : connections_.at(id)) {
        bool removeConnection = false;
        auto& id = connection_peer.first;
        auto& connection = connection_peer.second;

        if (connection.packets_waiting() >
				NetworkConstants::MAX_WAITING_PACKETS_PER_CLIENT)
            continue;

        if(FD_ISSET(connection.get_socket(), &error_set)) {
            Log(WARNING) << "errorSet was set in connection\n";

            removeConnection = true;
        }

        else if(FD_ISSET(connection.get_socket(), &read_set)) {
            int received = recv(connection.get_socket(), buffer, BUFFER_SIZE,
					0);

            if(received <= 0) {
                if(received == 0) {
                    Log(INFORMATION) << "Connection #" << connection.get_id()
							<< " disconnected\n";
                    removeConnection = true;
                }

                else {
                    if(errno == EWOULDBLOCK || errno == EAGAIN) {
                        Log(WARNING) << "EWOULDBLOCK activated\n";
                    }

                    else {
                        Log(WARNING) << "Connection #" << connection.get_id()
								<< " failed with errno = " << errno << '\n';
                        removeConnection = true;
                    }
                }
            }

            else {
                if (!CommonNetwork::assemble_packet(buffer, received,
						connection, incoming_mutex_, incoming_cv_,
						incoming_packets_)) {
                    Log(WARNING) << "Assembling packet from client failed," \
					"removing connection\n";
                    removeConnection = true;
                }
            }
        }

        if(removeConnection) {
            // Remove
            if (disconnect_function_)
                disconnect_function_(connection.get_socket(), id);
            else
                Log(WARNING) << "No valid disconnect function was registered\n";

            if (close(connection.get_socket()) < 0)
                Log(ERROR) << "close() got errno = " << errno << endl;

            connections_.at(id).erase(connection_peer.first);

            break;
        }
    }

    return true;
}

bool Server::select_send(fd_set& send_set, fd_set& error_set, int id,
		const unordered_set<int>& fds)
{
    if (!select(FD_SETSIZE, NULL, &send_set, &error_set, NULL)) {
        Log(ERROR) << "select() returned 0 when there's no timeout on sending" \
		" thread " << id << endl;
        return false;
    }

    lock_guard<mutex> lock(*outgoing_mutex_.at(id));

    // Iterate through file descriptors to see which is able to send
    for (auto& fd : fds) {
        if (FD_ISSET(fd, &error_set)) {
            // Remove all packets going to this file descriptor
            Log(DEBUG) << "Removing packets from " << fd << endl;

            outgoing_packets_.at(id).erase(remove_if(
					outgoing_packets_.at(id).begin(),
					outgoing_packets_.at(id).end(), [&fd] (auto& peer) {
                return fd == peer.first;
            }), outgoing_packets_.at(id).end());

            continue;
        }

        if (FD_ISSET(fd, &send_set)) {
            // It's possible to send something
            // Find a packet in the queue belonging to this descriptor
            //for (auto& peer : mOutgoingPackets.at(thread_id)) {
            for (auto iterator = outgoing_packets_.at(id).begin(); iterator != outgoing_packets_.at(id).end(); ++iterator) {
                auto& peer = *iterator;

                if (peer.first != fd)
                    continue;

                // Found one
                auto& packet = peer.second;

                int sending = min((unsigned int)NetworkConstants::BUFFER_SIZE, packet.getSize() - packet.getSent());
                int sent = send(fd, packet.getData() + packet.getSent(), sending, 0);

                bool removePacket = false;

                if(sent <= 0) {
                    if(sent == 0) {
                        removePacket = true;
                    }

                    else {
                        if(errno == EWOULDBLOCK || errno == EAGAIN) {
                            Log(WARNING) << "Send warning ";

                            if (errno == EWOULDBLOCK)
                                Log(NONE) << "EWOULDBLOCK\n";
                            else
                                Log(NONE) << "EAGAIN\n";

                            continue;
                        }

                        else {
                            removePacket = true;
                        }
                    }
                }

                else {
                    packet.addSent(sent);

                    if(packet.fullySent()) {
                        removePacket = true;
                    }
                }

                if(removePacket)
                    outgoing_packets_.at(id).erase(iterator);

                break;
            }
        }
    }

    return true;
}

void Server::register_disconnect_callback(function<void(int, size_t)> disconnect_function) {
    disconnect_function_ = disconnect_function;
}

void Server::wait_for_outgoing_packets(int id) {
    unique_lock<mutex> lock(*outgoing_mutex_.at(id));
    outgoing_cv_.at(id)->wait(lock, [this, &id] { return !outgoing_packets_.at(id).empty(); });
}

void Server::add_outgoing_packet(int fd, const Packet &packet, bool safe_send) {
    auto index = fd % num_send_threads_;

    Packet new_packet = packet;

    if (safe_send)
        new_packet.deepCopy(packet);

    lock_guard<mutex> guard(*outgoing_mutex_.at(index));
    outgoing_packets_.at(index).push_back({ fd, new_packet });
    outgoing_cv_.at(index)->notify_one();
}

void Server::send_socket(int fd, const Packet& packet, bool safe_send) {
    add_outgoing_packet(fd, packet, safe_send);
}

void Server::send_id(size_t id, const Packet& packet, bool safe_send) {
    auto fd = get_connection_fd(id);

    if (fd < 0) {
        Log(WARNING) << "Could not find connection with unique ID " << id << endl;
        return;
    }

    add_outgoing_packet(fd, packet, safe_send);
}

int Server::get_connection_fd(size_t unique_id) {
    for (size_t i = 0; i < connections_.size(); i++) {
        lock_guard<mutex> lock(*connections_mutex_.at(i));
        auto& queue = connections_.at(i);

        auto iterator = queue.find(unique_id);

        if (iterator != queue.end())
            return iterator->second.get_socket();
    }

    return -1;
}

bool Server::select_accept(fd_set& read_set, fd_set& error_set) {
    if (select(FD_SETSIZE, &read_set, NULL, &error_set, NULL) == 0) {
        Log(ERROR) << "select() returned 0 when there's no timeout\n";
        return false;
    }

    if (FD_ISSET(socket_, &error_set)) {
        Log(ERROR) << "masterSocket received an error\n";
        return false;
    }

    if (FD_ISSET(socket_, &read_set)) {
        sockaddr_in client;
        client.sin_family = AF_INET;
        socklen_t client_len = sizeof client;

        int newSocket = accept(socket_, (sockaddr*)&client, &client_len);
        if(newSocket < 0) {
            Log(ERROR) << "accept() failed with " << errno << endl;
            return false;
        }

        int index = newSocket % num_recv_threads_;
        string ip = inet_ntoa(client.sin_addr);

        {
			Log(NONE) << "USING " << index << " " << connections_mutex_.size() << endl;
            lock_guard<mutex> guard(*connections_mutex_.at(index));
			Log(NONE) << "AFTER";

            Connection connection(newSocket);
            connection.set_ip(ip);

            connections_.at(index).insert({ connection.get_id(), connection });

            Log(INFORMATION) << "Connection #" << connection.get_id() << " added from " << ip << "\n";
        }

        pipes_.at(index).set();
    }

    return true;
}

void Server::send_all_except(const Packet& packet, const std::vector<int>& except) {
    // Go through all receiving threads to find the connections
    for (size_t i = 0; i < connections_.size(); i++) {
        lock_guard<mutex> lock(*connections_mutex_.at(i));
        auto& connections = connections_.at(i);

        for (auto& connection_peer : connections) {
            if (!except.empty()) {
				if (find_if(except.begin(), except.end(), [&connection_peer] (auto fd) {
                        return connection_peer.second.get_socket() == fd; }) != except.end()) {
					continue;
                }
			}

            add_outgoing_packet(connection_peer.second.get_socket(), packet);
        }
    }
}

void Server::send_all(const Packet& packet) {
    send_all_except(packet, {});
}

pair<size_t, Packet> Server::wait_for_packets() {
    pair<size_t, Packet> packet;

    {
        unique_lock<mutex> lock(incoming_mutex_);
        incoming_cv_.wait(lock, [this] { return !incoming_packets_.empty(); });

        // Grab Packet
        packet = incoming_packets_.front();
        incoming_packets_.pop_front();
    }

    // Set Connection to successfully processed Packet thus decreasing waiting Packets counter
    auto id = packet.first;

    for (size_t i = 0; i < connections_.size(); i++) {
        lock_guard<mutex> lock(*connections_mutex_.at(i));
        auto& queue = connections_.at(i);

        auto iterator = queue.find(id);

        if (iterator == queue.end())
            continue;

        iterator->second.reduce_wait();
    }

    return packet;
}

string Server::get_ip(size_t id) const
{
    // Iterate through threads to find correct ID
    for (size_t i = 0; i < connections_.size(); i++) {
        lock_guard<mutex> lock(*connections_mutex_.at(i));
        auto& queue = connections_.at(i);

        auto iterator = queue.find(id);

        if (iterator == queue.end())
            continue;

        return iterator->second.get_ip();
    }

    return "not found";
}