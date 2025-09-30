#include "eventcore/net/buffer.h"
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>

namespace eventcore {
    namespace net {

        const char Buffer::kCRLF[] = "\r\n";

        Buffer::Buffer(size_t initial_size)
            : buffer_(kPrependSize + initial_size),
            read_index_(kPrependSize),
            write_index_(kPrependSize) {}

        void Buffer::retrieve(size_t len) {
            if (len < readable_bytes()) read_index_ += len;
            else retrieve_all();
        }

        void Buffer::retrieve_all() {
            read_index_ = kPrependSize;
            write_index_ = kPrependSize;
        }

        std::string Buffer::retrieve_as_string(size_t len) {
            std::string result(peek(), len);
            retrieve(len);
            return result;
        }

        std::string Buffer::retrieve_all_as_string() {
            return retrieve_as_string(readable_bytes());
        }

        void Buffer::append(const char* data, size_t len) {
            ensure_writable(len);
            std::copy(data, data + len, begin_write());
            has_written(len);
        }

        void Buffer::append(const std::string& str) {
            append(str.data(), str.size());
        }

        void Buffer::append(const void* data, size_t len) {
            append(static_cast<const char*>(data), len);
        }

        void Buffer::ensure_writable(size_t len) {
            if (writable_bytes() < len) make_space(len);
        }

        void Buffer::make_space(size_t len) {
            if (writable_bytes() + prependable_bytes() < len + kPrependSize) {
                buffer_.resize(write_index_ + len);
            } else {
                size_t readable = readable_bytes();
                std::copy(begin() + read_index_, begin() + write_index_, begin() + kPrependSize);
                read_index_ = kPrependSize;
                write_index_ = read_index_ + readable;
            }
        }

        ssize_t Buffer::read_from_fd(int fd) {
            char extrabuf[65536];
            struct iovec vec[2];
            const size_t writable = writable_bytes();

            vec[0].iov_base = begin_write();
            vec[0].iov_len = writable;
            vec[1].iov_base = extrabuf;
            vec[1].iov_len = sizeof(extrabuf);

            const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
            const ssize_t n = ::readv(fd, vec, iovcnt);

            if (n < 0) return n;
            else if (static_cast<size_t>(n) <= writable) write_index_ += n;
            else {
                write_index_ = buffer_.size();
                append(extrabuf, n - writable);
            }
            return n;
        }

        const char* Buffer::find_crlf() const {
            const char* crlf = std::search(peek(), begin_write(), kCRLF, kCRLF + 2);
            return crlf == begin_write() ? nullptr : crlf;
        }

        const char* Buffer::find_crlf(const char* start) const {
            const char* crlf = std::search(start, begin_write(), kCRLF, kCRLF + 2);
            return crlf == begin_write() ? nullptr : crlf;
        }

        const char* Buffer::find_eol() const {
            const void* eol = memchr(peek(), '\n', readable_bytes());
            return static_cast<const char*>(eol);
        }

        const char* Buffer::find_eol(const char* start) const {
            const void* eol = memchr(start, '\n', begin_write() - start);
            return static_cast<const char*>(eol);
        }

    } // namespace net
} // namespace eventcore
