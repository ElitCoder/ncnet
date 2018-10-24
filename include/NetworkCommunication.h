#ifndef NETWORK_COMMUNICATION_H
#define NETWORK_COMMUNICATION_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <list>
#include <unordered_map>
#include <functional>
#include <unordered_set>

enum NetworkConstants {
    BUFFER_SIZE = 1048576,
    MAX_WAITING_PACKETS_PER_CLIENT = 10
};

class EventPipe {
public:
    explicit EventPipe();
    ~EventPipe();

    void setPipe();
    void resetPipe();

    int getSocket();

private:
    int mPipes[2];
    std::shared_ptr<std::mutex> event_mutex_;
};

class Connection;
class Packet;
class PartialPacket;

class NetworkCommunication {
public:
    NetworkCommunication();
    ~NetworkCommunication();

    void start(unsigned short port, int num_sending_threads, int num_receiving_threads, bool stats = false, int delay = 3000);

    void setFileDescriptorsAccept(fd_set &readSet, fd_set &errorSet);
    void setFileDescriptorsReceive(fd_set &readSet, fd_set &errorSet, int thread_id);
    void setFileDescriptorsSend(fd_set& sendSet, fd_set& errorSet, int thread_id, std::unordered_set<int>& fds);

    bool runSelectAccept(fd_set &readSet, fd_set &errorSet);
    bool runSelectReceive(fd_set &readSet, fd_set &errorSet, unsigned char *buffer, int thread_id);
    bool runSelectSend(fd_set& sendSet, fd_set& errorSet, int thread_id, const std::unordered_set<int>& fds);

    int getSocket() const;
    int getConnectionSocket(size_t unique_id);
    std::string getIP(size_t unique_id) const;

    void waitForOutgoingPackets(int thread_id);
    void addOutgoingPacket(const int fd, const Packet &packet, bool safe_send = true);
    void sendFD(int fd, const Packet& packet, bool safe_send = true);
    void sendID(size_t id, const Packet& packet, bool safe_send = true);

    std::tuple<int, size_t, Packet> waitForProcessingPackets();

    void sendToAll(const Packet& packet);
    void sendToAllExcept(const Packet &packet, const std::vector<int> &except);

    std::vector<std::string> getStats();

    void registerDisconnectFunction(std::function<void(int, size_t)> disconnect_function);

private:
    bool assemblePacket(const unsigned char *buffer, const unsigned int received, Connection &connection);
    unsigned int processBuffer(const unsigned char *buffer, const unsigned int received, PartialPacket &partialPacket);
    void moveProcessedPacketsToQueue(Connection &connection);

    int mSocket;
    std::vector<EventPipe> mPipe;

    std::thread mAcceptThread, mStatsThread;
    std::vector<std::thread> mSendThread;
    std::vector<std::thread> mReceiveThread;

    std::mutex mIncomingMutex;
    std::condition_variable mIncomingCV;
    std::list<std::tuple<int, size_t, Packet>> mIncomingPackets;

    std::vector<std::shared_ptr<std::mutex>> mOutgoingMutex;
    std::vector<std::shared_ptr<std::condition_variable>> mOutgoingCV;
    std::vector<std::list<std::pair<int, Packet>>> mOutgoingPackets;

    std::vector<std::shared_ptr<std::mutex>> mConnectionsMutex;
    std::vector<std::unordered_map<size_t, Connection>> mConnections;

    int num_sending_threads_    = 1;
    int num_receiving_threads_  = 1;

    std::function<void(int, size_t)> disconnect_function_;
};

#endif