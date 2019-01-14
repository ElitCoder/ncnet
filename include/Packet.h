#ifndef PACKET_H
#define PACKET_H

#include <memory>
#include <vector>

class Packet {
public:
    // Common
    Packet();

    // Receiving
    void addRaw(const unsigned char* buffer, int size);
    bool hasFullSize() const;
    size_t getLeft() const;
    bool isFinished() const;

    // Sending
    unsigned char* getData();
    size_t getSent() const;
    bool addSent(int sent);
    size_t getSize() const;

private:
    // Common
    std::shared_ptr<std::vector<unsigned char>> data_;

    // Receiving
    size_t full_size_ = 0;

    // Sending
    size_t sent_ = 0;
};

#endif