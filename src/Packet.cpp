#include "Packet.h"
#include "InvalidPacketException.h"

using namespace std;

constexpr auto PACKET_HEADER_SIZE = 4;

Packet::Packet() {
    data_ = make_shared<vector<unsigned char>>();
}

void Packet::addRaw(const unsigned char* buffer, int size) {
    data_->insert(data_->end(), buffer, buffer + size);

    if (data_->size() > PACKET_HEADER_SIZE && full_size_ == 0) {
        // Decode integer to get full size
         full_size_ = (data_->at(0) << 24) | (data_->at(1) << 16) | (data_->at(2) << 8) | data_->at(3);

         if (full_size_ == 0) {
             // Don't accept empty packets
             throw InvalidPacketException();
         }

         try {
             data_->reserve(full_size_);
         } catch (...) {
             // Bad allocation
             throw InvalidPacketException();
         }
    }
}

bool Packet::hasFullSize() const {
    return full_size_ != 0;
}

size_t Packet::getLeft() const {
    if (full_size_ == 0) {
        return PACKET_HEADER_SIZE - data_->size();
    } else {
        return full_size_ - data_->size();
    }
}

bool Packet::isFinished() const {
    if (full_size_ == 0) {
        return false;
    }

    return full_size_ == data_->size();
}