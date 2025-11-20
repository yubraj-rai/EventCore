#pragma once
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>

namespace eventcore {
    namespace net {

        class Buffer {
            public:
                static constexpr size_t kInitialSize = 1024;
                static constexpr size_t kPrependSize = 8;
                static const char kCRLF[];

                explicit Buffer(size_t initial_size = kInitialSize);
                size_t readable_bytes() const { return write_index_ - read_index_; }
                size_t writable_bytes() const { return buffer_.size() - write_index_; }
                size_t prependable_bytes() const { return read_index_; }
                const char* peek() const { return begin() + read_index_; }

                void retrieve(size_t len);
                void retrieve_all();
                std::string retrieve_as_string(size_t len);
                std::string retrieve_all_as_string();
                void append(const char* data, size_t len);
                void append(const std::string& str);
                void append(const void* data, size_t len);
                void ensure_writable(size_t len);
                char* begin_write() { return begin() + write_index_; }
                const char* begin_write() const { return begin() + write_index_; }
                void has_written(size_t len) { write_index_ += len; }
                ssize_t read_from_fd(int fd);
                const char* find_crlf() const;
                const char* find_crlf(const char* start) const;
                const char* find_eol() const;
                const char* find_eol(const char* start) const;

            private:
                char* begin() { return &*buffer_.begin(); }
                const char* begin() const { return &*buffer_.begin(); }
                void make_space(size_t len);

                std::vector<char> buffer_;
                size_t read_index_;
                size_t write_index_;
        };

    } // namespace net
} // namespace eventcore
