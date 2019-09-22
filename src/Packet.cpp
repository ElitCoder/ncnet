#include "Packet.h"
#include "Log.h"

#include <cassert>

using namespace std;

namespace ncnet {
    void Packet::add_length_prefix(const string &val) {
        auto length = to_string(val.size());
        data_->push_back(length.size());
        data_->insert(data_->end(), length.begin(), length.end());
    }

    void Packet::handle_error(const string &message) {
        Log(ERROR) << "Error in packet (" << message << "), exiting";
        assert(false);
    }

#if 0
    Packet::Packet() {
        data_ = make_shared<vector<unsigned char>>();
    }

    void Packet::addRaw(const unsigned char* buffer, int size) {
        // FIXME: Maybe check for inserting more than necessary? Should be covered though
        data_->insert(data_->end(), buffer, buffer + size);
        Log(DEBUG) << "Inserting " << size << " bytes";

        if (data_->size() >= PACKET_HEADER_SIZE && full_size_ == 0) {
            // Decode integer to get full size
            full_size_ = (data_->at(0) << 24) | (data_->at(1) << 16) | (data_->at(2) << 8) | data_->at(3);
            Log(DEBUG) << "Decoding packet to " << full_size_ << " bytes size";

            if (full_size_ == 0) {
                // Don't accept empty packets
                throw exception();
            }

            try {
                data_->reserve(full_size_);
            } catch (...) {
                // Bad allocation
                throw exception();
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

    unsigned char* Packet::getData() {
        return data_->data();
    }

    size_t Packet::getSent() const {
        return sent_;
    }

    bool Packet::addSent(int sent) {
        sent_ += sent;

        return sent_ >= data_->size();
    }

    size_t Packet::getSize() const {
        return data_->size();
    }

    bool Packet::isFinalized() const {
        return finalized_;
    }

    void Packet::addHeader(unsigned char header) {
        if (isFinalized()) {
            assert(0); // Mistake
        }

        // Add size header placeholder
        data_->resize(PACKET_HEADER_SIZE);
        fill(data_->begin(), data_->end(), 0);

        // Add header
        data_->push_back(header);
    }

    void Packet::addInt(int value) {
        if (isFinalized()) {
            assert(0); // Mistake
        }

        data_->push_back((value >> 24) & 0xFF);
        data_->push_back((value >> 16) & 0xFF);
        data_->push_back((value >> 8) & 0xFF);
        data_->push_back(value & 0xFF);
    }

    void Packet::addString(const string& str) {
        if (isFinalized()) {
            assert(0); // Mistake
        }

        addInt(str.length());
        data_->insert(data_->end(), str.begin(), str.end());
    }

    void Packet::addBool(bool value) {
        if (isFinalized()) {
            assert(0); // Mistake
        }

        data_->push_back(value);
    }

    unsigned char Packet::getHeader() {
        return data_->at(read_++);
    }

    int Packet::getInt() {
        int value = (data_->at(read_) << 24)        |
                    (data_->at(read_ + 1) << 16)    |
                    (data_->at(read_ + 2) << 8)     |
                    (data_->at(read_ + 3));
        read_ += 4;
        return value;
    }

    string Packet::getString() {
        auto length = getInt();
        string str(data_->begin() + read_, data_->begin() + read_ + length);
        read_ += length;
        return str;
    }

    bool Packet::getBool() {
        return data_->at(read_++);
    }

    void Packet::finalize() {
        if (isFinalized()) {
            assert(0);
        }

        for (int i = 0; i < PACKET_HEADER_SIZE; i++) {
            data_->at(i) = data_->size() >> (24 - i * 8) & 0xFF;
        }

        finalized_ = true;
    }
#endif
}