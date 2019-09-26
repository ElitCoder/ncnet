#pragma once

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
        unsigned char *get_send_buffer(); // Get current array for sending
        size_t left_to_send() const; // Bytes left to send
        bool sent_data(size_t sent); // Sent bytes

        // Creating
        void finalize(); // Calculate header size and packet data

        Packet &operator<<(const std::string &val) {
            create_add_string(val);
            return *this;
        }

        Packet &operator<<(std::string val) {
            create_add_string(val);
            return *this;
        }

        template<class T>
        Packet &operator<<(const T &val) {
            create_add_var(val); // Return value is catched by assert anyway
            return *this;
        }

        template<class T>
        Packet &operator<<(T val) {
            create_add_var(val);
            return *this;
        }

        // Reading
#if 0
        template<class T>
        Packet &operator>>(const std::string &val) {
        }
#endif

        template<class T>
        Packet &operator>>(T &val) {
            // Read prefix length
            auto prefix_length = data_->at(read_position_++);
            auto prefix_str = std::string(data_->begin() + read_position_, data_->begin() + read_position_ + prefix_length);
            read_position_ += prefix_length;

            // String length -> size_t
            std::stringstream length_stream(prefix_str);
            size_t actual_length;
            length_stream >> actual_length;
            if (length_stream.fail()) {
                handle_error("Could not read actual string length");
                return *this;
            }

            // Read final string
            auto actual_str = std::string(data_->begin() + read_position_, data_->begin() + read_position_ + actual_length);
            read_position_ += actual_length;

            // Transform to T
            std::stringstream actual_stream(actual_str);
            actual_stream >> std::skipws >> val;
            if (actual_stream.fail()) {
                handle_error("Could not read actual string");
            }

            return *this;
        }

    private:
        bool create_add_string(const std::string &val) {
            if (fixed_) {
                handle_error("Packet already fixed");
                return false;
            }

            // Add string length and string
            add_length_prefix(val);
            data_->insert(data_->end(), val.begin(), val.end());
        }

        template<class T>
        bool create_add_val(const T &val) {
            if (fixed_) {
                handle_error("Packet already fixed");
                return false;
            }

            // Convert
            std::ostringstream stream;
            stream << val;
            if (stream.fail()) {
                handle_error("Input type is not valid for sending");
                return false;
            }

            // Add string length
            auto &string_val = stream.str();
            add_length_prefix(string_val);

            // Add var
            data_->insert(data_->end(), string_val.begin(), string_val.end());
            return true;
        }

        // Prefix inserted value with string length as in [length prefix length][length prefix][actual string]
        void add_length_prefix(const std::string &val);
        void handle_error(const std::string &message) const; // Do something clever with errors

        // Common
        DataType data_;

        // Creating
        bool fixed_ = false; // Allow no more insertions

        // Receiving
        size_t added_ = 0; // Actually received bytes
        size_t full_size_ = 0;

        // Sending
        size_t sent_ = 0; // Actually sent bytes

        // Reading
        size_t read_position_ = PACKET_HEADER_SIZE; // Current reading position
    };
}