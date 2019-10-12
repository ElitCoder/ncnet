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

    void Packet::read_string(string &val) {
        // Read prefix length
        auto prefix_len = data_->at(read_position_++);
        auto prefix = string(data_->begin() + read_position_, data_->begin() + read_position_ + prefix_len);
        read_position_ += prefix_len;
        // Read string
        auto len = stoull(prefix);
        val = string(data_->begin() + read_position_, data_->begin() + read_position_ + len);
        read_position_ += len;
    }

    unsigned char Packet::read_byte() {
        return data_->at(read_position_++);
    }

    void Packet::add_string(const string &val) {
        // Add prefix
        auto prefix = to_string(val.length());
        data_->push_back(prefix.length());
        data_->insert(data_->end(), prefix.begin(), prefix.end());
        // Add string
        data_->insert(data_->end(), val.begin(), val.end());
    }

    void Packet::add_byte(unsigned char val) {
        data_->push_back(val);
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

    void Packet::set_packet_size() {
        for (int i = 0; i < PACKET_HEADER_SIZE; i++) {
            data_->at(i) = data_->size() >> (24 - i * 8) & 0xFF;
        }
    }

    void Packet::finalize() {
        if (fixed_) {
            return;
        }

        set_packet_size();
        Log(DEBUG) << "Finalizing packet with size " << data_->size() << " and content ";
        fixed_ = true;
    }

    void Packet::encrypt(Security &security) {
        // Encrypted content
        DataType new_data = make_shared<vector<unsigned char>>();
        new_data->resize(PACKET_HEADER_SIZE); // Reserve size of packet
        // Encrypt current data vector and store it in new_data
        security.encrypt(data_, PACKET_HEADER_SIZE, new_data);
        // Replace
        data_ = new_data;
        // Recalculate size
        set_packet_size();
    }

    void Packet::decrypt(Security &security) {
        // Remove extra allocation for decryption to match
        data_->resize(added_);
        // Decrypted content
        DataType new_data = make_shared<vector<unsigned char>>();
        new_data->resize(PACKET_HEADER_SIZE); // Reserve size of packet
        security.decrypt(data_, PACKET_HEADER_SIZE, new_data);
        data_ = new_data;
        // Recalculate size
        set_packet_size();
    }

    // Adding
    Packet &Packet::operator<<(bool val) {
        add_data(val);
        return *this;
    }
    Packet &Packet::operator<<(short val) {
        add_data(val);
        return *this;
    }
    Packet &Packet::operator<<(unsigned short val) {
        add_data(val);
        return *this;
    }
    Packet &Packet::operator<<(int val) {
        add_data(val);
        return *this;
    }
    Packet &Packet::operator<<(unsigned int val) {
        add_data(val);
        return *this;
    }
    Packet &Packet::operator<<(long val) {
        add_data(val);
        return *this;
    }
    Packet &Packet::operator<<(unsigned long val) {
        add_data(val);
        return *this;
    }
    Packet &Packet::operator<<(long long val) {
        add_data(val);
        return *this;
    }
    Packet &Packet::operator<<(unsigned long long val) {
        add_data(val);
        return *this;
    }
    Packet &Packet::operator<<(float val) {
        add_data(val);
        return *this;
    }
    Packet &Packet::operator<<(double val) {
        add_data(val);
        return *this;
    }
    Packet &Packet::operator<<(long double val) {
        add_data(val);
        return *this;
    }
    Packet &Packet::operator<<(const std::string &val) {
        add_string(val);
        return *this;
    }
    Packet &Packet::operator<<(const char *val) {
        add_string(string(val));
        return *this;
    }

    // Reading
    Packet &Packet::operator>>(bool &val) {
        read_data(val);
        return *this;
    }
    Packet &Packet::operator>>(short &val) {
        read_data(val);
        return *this;
    }
    Packet &Packet::operator>>(unsigned short &val) {
        read_data(val);
        return *this;
    }
    Packet &Packet::operator>>(int &val) {
        read_data(val);
        return *this;
    }
    Packet &Packet::operator>>(unsigned int &val) {
        read_data(val);
        return *this;
    }
    Packet &Packet::operator>>(long &val) {
        read_data(val);
        return *this;
    }
    Packet &Packet::operator>>(unsigned long &val) {
        read_data(val);
        return *this;
    }
    Packet &Packet::operator>>(long long &val) {
        read_data(val);
        return *this;
    }
    Packet &Packet::operator>>(unsigned long long &val) {
        read_data(val);
        return *this;
    }
    Packet &Packet::operator>>(float &val) {
        read_data(val);
        return *this;
    }
    Packet &Packet::operator>>(double &val) {
        read_data(val);
        return *this;
    }
    Packet &Packet::operator>>(long double &val) {
        read_data(val);
        return *this;
    }
    Packet &Packet::operator>>(std::string &val) {
        read_string(val);
        return *this;
    }
}