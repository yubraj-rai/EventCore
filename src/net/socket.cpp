#include "eventcore/net/socket.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

namespace eventcore {
    namespace net {

        Socket::Socket() : fd_(-1) {}
        Socket::Socket(int fd) : fd_(fd) {}

        Socket::~Socket() {
            if (fd_ >= 0) ::close(fd_);
        }

        Socket::Socket(Socket&& other) noexcept : fd_(other.fd_) {
            other.fd_ = -1;
        }

        Socket& Socket::operator=(Socket&& other) noexcept {
            if (this != &other) {
                close();
                fd_ = other.fd_;
                other.fd_ = -1;
            }
            return *this;
        }
        Result<void> Socket::bind(const Address& addr) {
            if (::bind(fd_, addr.sockaddr(), addr.socklen()) < 0) {
                return Result<void>::Err(std::string("bind failed: ") + strerror(errno));
            }
            return Result<void>::Ok();
        }

        Result<void> Socket::listen(int backlog) {
            if (::listen(fd_, backlog) < 0) {
                return Result<void>::Err(std::string("listen failed: ") + strerror(errno));
            }
            return Result<void>::Ok();
        }

        Result<void> Socket::connect(const Address& addr) {
            if (::connect(fd_, addr.sockaddr(), addr.socklen()) < 0) {
                return Result<void>::Err(std::string("connect failed: ") + strerror(errno));
            }
            return Result<void>::Ok();
        }

        Result<Socket> Socket::accept() {
            struct sockaddr_in addr;
            socklen_t len = sizeof(addr);
            int client_fd = ::accept(fd_, reinterpret_cast<struct sockaddr*>(&addr), &len);
            if (client_fd < 0) {
                return Result<Socket>::Err(std::string("accept failed: ") + strerror(errno));
            }
            return Result<Socket>::Ok(Socket(client_fd));
        }


        Result<size_t> Socket::send(const void* data, size_t len) {
            ssize_t n = ::send(fd_, data, len, MSG_NOSIGNAL);
            if (n < 0) return Result<size_t>::Err(std::string("send failed: ") + strerror(errno));
            return Result<size_t>::Ok(static_cast<size_t>(n));
        }

        Result<size_t> Socket::recv(void* data, size_t len) {
            ssize_t n = ::recv(fd_, data, len, 0);
            if (n < 0) return Result<size_t>::Err(std::string("recv failed: ") + strerror(errno));
            return Result<size_t>::Ok(static_cast<size_t>(n));
        }


        Result<void> Socket::set_nonblocking(bool enable) {
            int flags = fcntl(fd_, F_GETFL, 0);
            if (flags < 0) {
                return Result<void>::Err("fcntl F_GETFL failed");
            }

            if (enable) {
                flags |= O_NONBLOCK;
            } else {
                flags &= ~O_NONBLOCK;
            }

            if (fcntl(fd_, F_SETFL, flags) < 0) {
                return Result<void>::Err("fcntl F_SETFL failed");
            }
            return Result<void>::Ok();
        }

        Result<void> Socket::set_reuseaddr(bool enable) {
            int optval = enable ? 1 : 0;
            if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
                return Result<void>::Err("setsockopt SO_REUSEADDR failed");
            }
            return Result<void>::Ok();
        }

        Result<void> Socket::set_reuseport(bool enable) {
#ifdef SO_REUSEPORT
            int optval = enable ? 1 : 0;
            if (setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
                return Result<void>::Err("setsockopt SO_REUSEPORT failed");
            }
#endif
            return Result<void>::Ok();
        }

        Result<void> Socket::set_nodelay(bool enable) {
            int optval = enable ? 1 : 0;
            if (setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) < 0) {
                return Result<void>::Err("setsockopt TCP_NODELAY failed");
            }
            return Result<void>::Ok();
        }

        Result<void> Socket::set_keepalive(bool enable) {
            int optval = enable ? 1 : 0;
            if (setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0) {
                return Result<void>::Err("setsockopt SO_KEEPALIVE failed");
            }
            return Result<void>::Ok();
        }
        void Socket::shutdown_write() {
            ::shutdown(fd_, SHUT_WR);
        }

        void Socket::close() {
            if (fd_ >= 0) {
                ::close(fd_);
                fd_ = -1;
            }
        }

        Result<Socket> Socket::create_tcp() {
            int fd = ::socket(AF_INET, SOCK_STREAM, 0);
            if (fd < 0) return Result<Socket>::Err(std::string("socket creation failed: ") + strerror(errno));
            return Result<Socket>::Ok(Socket(fd));
        }

        Result<Socket> Socket::create_udp() {
            int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
            if (fd < 0) return Result<Socket>::Err(std::string("socket creation failed: ") + strerror(errno));
            return Result<Socket>::Ok(Socket(fd));
        }

    } // namespace net
} // namespace eventcore
