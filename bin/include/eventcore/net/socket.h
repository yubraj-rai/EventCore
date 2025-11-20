#pragma once
#include "../core/noncopyable.h"
#include "../core/result.h"
#include "address.h"
#include <string>
#include <cstdint>

namespace eventcore {
    namespace net {

        class Socket : public NonCopyable {
            public:
                Socket();
                explicit Socket(int fd);
                ~Socket();
                Socket(Socket&& other) noexcept;
                Socket& operator=(Socket&& other) noexcept;

                Result<void> bind(const Address& addr);
                Result<void> listen(int backlog = 1024);
                Result<Socket> accept();
                Result<void> connect(const Address& addr);
                Result<size_t> send(const void* data, size_t len);
                Result<size_t> recv(void* data, size_t len);
                Result<void> set_nonblocking(bool enable = true);
                Result<void> set_reuseaddr(bool enable = true);
                Result<void> set_reuseport(bool enable = true);
                Result<void> set_nodelay(bool enable = true);
                Result<void> set_keepalive(bool enable = true);
                void shutdown_write();
                void close();

                int fd() const { return fd_; }
                bool is_valid() const { return fd_ >= 0; }
                static Result<Socket> create_tcp();
                static Result<Socket> create_udp();

            public:
                int release() {
                    int fd = fd_;
                    fd_ = -1;
                    return fd;
                }

            private:
                int fd_ = -1;
        };

    } // namespace net
} // namespace eventcore
