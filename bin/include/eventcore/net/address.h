#pragma once
#include <string>
#include <cstdint>
#include <netinet/in.h>

namespace eventcore {
    namespace net {

        class Address {
            public:
                Address() = default;
                Address(const std::string& ip, uint16_t port);
                explicit Address(const struct sockaddr_in& addr);

                std::string ip() const;
                uint16_t port() const;
                std::string to_string() const;
                const struct sockaddr* sockaddr() const;
                socklen_t socklen() const;
                static Address from_sockaddr(const struct sockaddr_in& addr);

            private:
                struct sockaddr_in addr_;
        };

    } // namespace net
} // namespace eventcore
