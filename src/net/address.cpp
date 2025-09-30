#include "eventcore/net/address.h"
#include <arpa/inet.h>
#include <cstring>
#include <sstream>

namespace eventcore {
    namespace net {

        Address::Address(const std::string& ip, uint16_t port) {
            std::memset(&addr_, 0, sizeof(addr_));
            addr_.sin_family = AF_INET;
            addr_.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
        }

        Address::Address(const struct sockaddr_in& addr) : addr_(addr) {}

        std::string Address::ip() const {
            char buf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
            return buf;
        }

        uint16_t Address::port() const {
            return ntohs(addr_.sin_port);
        }

        std::string Address::to_string() const {
            std::stringstream ss;
            ss << ip() << ":" << port();
            return ss.str();
        }

        const struct sockaddr* Address::sockaddr() const {
            return reinterpret_cast<const struct sockaddr*>(&addr_);
        }

        socklen_t Address::socklen() const {
            return sizeof(addr_);
        }

        Address Address::from_sockaddr(const struct sockaddr_in& addr) {
            return Address(addr);
        }

    } // namespace net
} // namespace eventcore
