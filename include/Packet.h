#ifndef PACKET_H
#define PACKET_H

#include <memory>
#include <vector>

constexpr auto PACKET_HEADER_SIZE = 4;

class Packet {
public:
    // Common
    Packet();

    // Receiving
    void addRaw(const unsigned char* buffer, int size);
    bool hasFullSize() const;
    size_t getLeft() const;
    bool isFinished() const;

    // Creating
    void addHeader(unsigned char header);
    void addInt(int value);
    unsigned char getHeader();
    int getInt();
    void finalize();

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

    // Creating
    bool isFinalized() const;
    bool finalized_ = false;

    // Reading
    size_t read_ = PACKET_HEADER_SIZE;

    // Sending
    size_t sent_ = 0;
};

#endif