#pragma once

#include "Security.h"

#include <memory>
#include <vector>
#include <sstream>

namespace ncnet {
    using DataType = std::shared_ptr<std::vector<unsigned char>>;

    constexpr auto PACKET_HEADER_SIZE = 4; // Limits to archs where sizeof(int) == 4
    constexpr auto MEMORY_DEFAULT_SIZE = 64 * 1024; // 64 KB

    class Packet {
    public:
        // Common
        explicit Packet();

        // Receiving
        unsigned char *get_writable_buffer(size_t size); // Returns allocated buffer
        bool added_data(size_t size); // How much data was inserted? Disconnect on false
        size_t left_in_packet() const; // Bytes left to receive
        bool has_received_full_packet() const; // If full packet is received

        // Sending
        void finalize(); // Calculate header size and packet data for sending
        unsigned char *get_send_buffer(); // Get current array for sending
        size_t left_to_send() const; // Bytes left to send
        bool sent_data(size_t sent); // Sent bytes

        // Adding
        template<class T>
        void add_data(T val) {
            auto str = std::to_string(val);
            data_->push_back(str.length());
            data_->insert(data_->end(), str.begin(), str.end());
        }

        void add_string(const std::string &val);
        void add_byte(unsigned char val);
        Packet &operator<<(bool val);
        Packet &operator<<(short val);
        Packet &operator<<(unsigned short val);
        Packet &operator<<(int val);
        Packet &operator<<(unsigned int val);
        Packet &operator<<(long val);
        Packet &operator<<(unsigned long val);
        Packet &operator<<(long long val);
        Packet &operator<<(unsigned long long val);
        Packet &operator<<(float val);
        Packet &operator<<(double val);
        Packet &operator<<(long double val);
        Packet &operator<<(const std::string &val);
        Packet &operator<<(const char *val);

        // Reading
        template<class T>
        void read_data(T &val) {
            auto len = data_->at(read_position_++);
            auto str = std::string(data_->begin() + read_position_, data_->begin() + read_position_ + len);
            read_position_ += len;

            // Convert
            std::istringstream stream(str);
            stream >> val;

            if (stream.fail()) {
                handle_error("Failed to convert data type");
            }
        }

        void read_string(std::string &val);
        unsigned char read_byte();
        Packet &operator>>(bool &val);
        Packet &operator>>(short &val);
        Packet &operator>>(unsigned short &val);
        Packet &operator>>(int &val);
        Packet &operator>>(unsigned int &val);
        Packet &operator>>(long &val);
        Packet &operator>>(unsigned long &val);
        Packet &operator>>(long long &val);
        Packet &operator>>(unsigned long long &val);
        Packet &operator>>(float &val);
        Packet &operator>>(double &val);
        Packet &operator>>(long double &val);
        Packet &operator>>(std::string &val);

        void encrypt(Security &security);
        void decrypt(Security &security);

    private:
        void set_packet_size(); // Calculate the packet size
        void handle_error(const std::string &message) const; // Do something clever with errors

        // Common
        DataType data_;

        // Receiving
        size_t added_ = 0; // Actually received bytes
        size_t full_size_ = 0;

        // Sending
        size_t sent_ = 0; // Actually sent bytes

        // Adding
        bool fixed_ = false; // Allow no more insertions

        // Reading
        size_t read_position_ = PACKET_HEADER_SIZE; // Current reading position
    };
}