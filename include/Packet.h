#ifndef PACKET_H
#define PACKET_H

#include <memory>
#include <vector>

class Packet {
public:
    Packet();

    void addRaw(const unsigned char* buffer, int size);
    bool hasFullSize() const;
    size_t getLeft() const;
    bool isFinished() const;

private:
    std::shared_ptr<std::vector<unsigned char>> data_;
    size_t full_size_ = 0;
};

#endif