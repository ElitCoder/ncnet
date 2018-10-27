#ifndef SERVER_H
#define SERVER_H

#include "EventPipe.h"
#include "Packet.h"
#include "Connection.h"

#include <vector>
#include <thread>
#include <condition_variable>
#include <list>
#include <unordered_map>
#include <functional>
#include <unordered_set>

class Server {
public:
	~Server();

	bool start(int port);
	bool setup(int port);

	void send_socket(int fd, const Packet& packet, bool safe_send = true);
	void send_id(size_t id, const Packet& packet, bool safe_send = true);
	void send_all(const Packet& packet);
	void send_all_except(const Packet& packet, const std::vector<int>& except);

	std::pair<size_t, Packet> wait_for_packets();

	std::string get_ip(size_t id) const;
	void register_disconnect_callback(
			std::function<void(int, size_t)> disconnect_function);

	void set_fd_accept(fd_set& recv_set, fd_set& error_set);
	void set_fd_recv(fd_set& read_set, fd_set& error_set, int id);
	void set_fd_send(fd_set& send_set, fd_set& error_set, int id,
			std::unordered_set<int>& fds);

	bool select_accept(fd_set& read_set, fd_set& error_set);
	bool select_recv(fd_set& read_set, fd_set& error_set,
			unsigned char *buffer, int id);
	bool select_send(fd_set& send_set, fd_set& error_set, int id,
			const std::unordered_set<int>& fds);

	void wait_for_outgoing_packets(int id);

private:
	void add_outgoing_packet(int fd, const Packet &packet,
			bool safe_send = true);
	int get_connection_fd(size_t id);

	int socket_;

	std::vector<EventPipe> pipes_;

    std::thread accept_thread_;
	std::thread stats_thread_;
    std::vector<std::thread> send_threads_;
    std::vector<std::thread> recv_threads_;

    std::mutex incoming_mutex_;
    std::condition_variable incoming_cv_;
    std::list<std::pair<size_t, Packet>> incoming_packets_;

    std::vector<std::shared_ptr<std::mutex>> outgoing_mutex_;
    std::vector<std::shared_ptr<std::condition_variable>> outgoing_cv_;
    std::vector<std::list<std::pair<int, Packet>>> outgoing_packets_;

    std::vector<std::shared_ptr<std::mutex>> connections_mutex_;
    std::vector<std::unordered_map<size_t, Connection>> connections_;

    std::function<void(int, size_t)> disconnect_function_;

	// TODO: Add back multithread support
	int num_recv_threads_ = 1;
	int num_send_threads_ = 1;
};

#endif