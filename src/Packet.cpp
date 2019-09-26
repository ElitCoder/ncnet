#include "Packet.h"
#include "Log.h"

#include <cassert>

using namespace std;

namespace ncnet {
    Packet::Packet() {
        // Create data
        data_ = make_shared<vector<unsigned char>>();
        data_->resize(PACKET_HEADER_SIZE); // Allocate header
    }

    void Packet::add_length_prefix(const string &val) {
        auto length = to_string(val.length());
        data_->push_back(length.length());
        data_->insert(data_->end(), length.begin(), length.end());
    }

    void Packet::handle_error(const string &message) const {
        Log(ERROR) << "Error in packet (" << message << "), exiting";
        assert(false);
    }

    bool Packet::has_received_full_packet() const {
        return full_size_ == 0 ? false : full_size_ == added_;
    }

    size_t Packet::left_in_packet() const {
        if (full_size_ == 0) {
            return PACKET_HEADER_SIZE - added_;
        } else {
            return full_size_ - added_;
        }
    }

    unsigned char *Packet::get_writable_buffer(size_t size) {
        // Need to re-allocate if added + size > data_->size()
        if (added_ + size > data_->size()) {
            try {
                data_->resize(added_ + size);
            } catch (...) {
                // Bad packet size
                throw exception();
            }
        }
        return data_->data() + added_;
    }

    bool Packet::added_data(size_t size) {
        if (added_ + size >= PACKET_HEADER_SIZE && full_size_ == 0) {
            // Decode header to get full size
            full_size_ = (data_->at(0) << 24) | (data_->at(1) << 16) | (data_->at(2) << 8) | data_->at(3);
            Log(DEBUG) << "Decoding packet to " << full_size_ << " bytes size";

            if (full_size_ == 0) {
                Log(ERROR) << "Connection tries to send empty packet, disconnecting";
                return false;
            }
        }

        // Data was received
        added_ += size;
        return true;
    }

    unsigned char *Packet::get_send_buffer() {
        return data_->data() + sent_;
    }

    size_t Packet::left_to_send() const {
        return data_->size() - sent_;
    }

    bool Packet::sent_data(size_t sent) {
        sent_ += sent;
        return sent_ == data_->size();
    }

    void Packet::finalize() {
        if (fixed_) {
            return;
        }

        for (int i = 0; i < PACKET_HEADER_SIZE; i++) {
            data_->at(i) = data_->size() >> (24 - i * 8) & 0xFF;
        }
        Log(DEBUG) << "Finalizing packet with size " << data_->size() << " and content ";
        fixed_ = true;
    }
}