#include "CommonNetwork.h"
#include "PartialPacket.h"
#include "Log.h"
#include "Connection.h"
#include "Packet.h"

using namespace std;

int CommonNetwork::process_buffer(const unsigned char *buffer, int received,
		PartialPacket &partial)
{
    if (partial.hasHeader()) {
        int insert = received;

        if (partial.getSize() + received >= partial.getFullSize()) {
            insert = partial.getFullSize() - partial.getSize();
        }

        if (insert > received) {
            Log(ERROR) << "Trying to insert more data than available in " \
					"buffer, insert = " << insert << ", received = " <<
					received << endl;
        }

        try {
            // In case we get bad allocation
            partial.addData(buffer, insert);
        } catch (...) {
            Log(ERROR) << "Ill-formed packet was received, avoiding bad " \
			"allocation\n";
            return -1;
        }

        return insert;
    }

    else {
        int left = 4 - partial.getSize();
        int adding = received > left ? left : received;

        if (adding > received) {
            Log(ERROR) << "Trying to insert more data than available in buffer" \
			", adding = " << adding << ", received = " << received << endl;
        }

        try {
            // In case we get bad allocation
            partial.addData(buffer, adding);
        } catch (...) {
            Log(ERROR) << "Ill-formed packet was received, avoiding bad " \
			"allocation\n";
            return -1;
        }

        return adding;
    }
}

bool CommonNetwork::assemble_packet(const unsigned char *buffer,
		int received, Connection &connection, mutex& incoming_mutex,
		condition_variable& incoming_cv,
		list<pair<size_t, Packet>>& incoming_packets)
{
    int processed = 0;

    do {
        int result = process_buffer(buffer + processed, received - processed,
				connection.get_partial_packet());
        if (result < 0) {
            return false;
        }

        processed += result;
    } while (processed < received);

    move_packets_to_queue(connection, incoming_mutex, incoming_cv,
			incoming_packets);
    return true;
}

void CommonNetwork::move_packets_to_queue(Connection& connection,
		mutex& incoming_mutex, condition_variable& incoming_cv,
		list<pair<size_t, Packet>>& incoming_packets)
{
    if (!connection.has_incoming_packet()) {
        return;
    }

    lock_guard<mutex> guard(incoming_mutex);

    while (connection.has_incoming_packet()) {
        incoming_packets.push_back({ connection.get_id(),
				move(connection.get_incoming_packet()) });
        connection.processed_packet();
    }

    incoming_cv.notify_one();
}