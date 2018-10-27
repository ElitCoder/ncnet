#ifndef COMMON_NETWORK_H
#define COMMON_NETWORK_H

#include <mutex>
#include <condition_variable>
#include <list>

enum NetworkConstants {
    BUFFER_SIZE = 1048576, // 1 MB
    MAX_WAITING_PACKETS_PER_CLIENT = 10
};

class PartialPacket;
class Connection;
class Packet;

class CommonNetwork {
public:
	static int process_buffer(const unsigned char *buffer,
			int received, PartialPacket& partial);

	static bool assemble_packet(const unsigned char *buffer,
			int received, Connection& connection,
			std::mutex& incoming_mutex, std::condition_variable& incoming_cv,
			std::list<std::pair<size_t, Packet>>& incoming_packets);

	static void move_packets_to_queue(Connection& connection,
			std::mutex& incoming_mutex, std::condition_variable& incoming_cv,
			std::list<std::pair<size_t, Packet>>& incoming_packets);
};

#endif