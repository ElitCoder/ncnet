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
        unsigned char *get_writable_buffer(size_t min_size); // Returns allocated buffer
        void buffer_position_changed(size_t size); // How much data was inserted?
        size_t left_in_packet() const;

        // Creating
        template<class T>
        Packet &operator<<(T &val) {
            std::stringstream stream;
            stream << std::skipws << val;
            if (stream.fail()) {
                // Data type not supported
                // Insert empty string
                handle_error("Invalid input type");
                return *this;
            }

            // Otherwise output to string and add
            std::string string_val;
            stream >> std::skipws >> string_val;
            add_length_prefix(string_val);
            data_->insert(data_->end(), string_val.begin(), string_val.end());
        }

        // Reading
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
        // Prefix inserted value with string length as in [length prefix length][length prefix][actual string]
        void add_length_prefix(const std::string &val);
        void handle_error(const std::string &message); // Do something clever with errors

        DataType data_;

        size_t read_position_ = 0; // Current reading position
    };

#if 0
    class Packet {
    public:
        // Common
        explicit Packet();

        // Creating
        template<class T>
        Packet& operator<<(T &val) {
            std::stringstream stream;
            stream << val;
            if (stream.fail()) {
                // Data type not supported
                // Insert empty string
                handle_error("Invalid input type");
                return *this;
            }
            // Otherwise output to string and add
            std::string string_val;
            stream >> string_val;
            auto &data = get_writable_data();
            add_length_prefix(data, string_val);
            data->insert(data.end(), string_val.begin(), string_val.end());
        }

        // Reading
        // TODO

    private:
        // Prefix inserted value with string length as in [length prefix length][length prefix][actual string]
        void add_length_prefix(DataType &data, const std::string &val);
        void handle_error(const std::string &message); // Do something clever with errors

        DataType &get_readable_data(); // Returns a readable vector
        DataType &get_writable_data(); // Returns a writable vector

        std::vector<Memory> data_; // Data pointers

        bool fixed_ = false; // Header size calculated

    };
#endif
#if 0
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
        void addString(const std::string &str);
        void addBool(bool value);
        unsigned char getHeader();
        int getInt();
        std::string getString();
        bool getBool();
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
}