#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

class TCPClient {
    private:
        int sockfd;

    public:
        TCPClient() : sockfd(-1) {}

        ~TCPClient() {
            if (sockfd >= 0) {
                close(sockfd);
            }
        }

        bool connect(const char* host, int port, bool no_delay = true) {
            // Create socket
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                std::cerr << "Failed to create socket" << std::endl;
                return false;
            }

            // Set TCP_NODELAY before connecting
            if (no_delay) {
                int flag = 1;
                if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, 
                            &flag, sizeof(int)) < 0) {
                    std::cerr << "Failed to set TCP_NODELAY" << std::endl;
                    close(sockfd);
                    sockfd = -1;
                    return false;
                }
            }

            // Setup server address
            struct sockaddr_in server_addr;
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);

            if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
                std::cerr << "Invalid address" << std::endl;
                close(sockfd);
                sockfd = -1;
                return false;
            }

            // Connect to server
            if (::connect(sockfd, (struct sockaddr*)&server_addr, 
                        sizeof(server_addr)) < 0) {
                std::cerr << "Connection failed" << std::endl;
                close(sockfd);
                sockfd = -1;
                return false;
            }

            std::cout << "Connected to " << host << ":" << port << std::endl;
            std::cout << "TCP_NODELAY: " << (no_delay ? "ON" : "OFF") << std::endl;
            return true;
        }

        ssize_t send(const char* data, size_t len) {
            return ::send(sockfd, data, len, 0);
        }

        ssize_t receive(char* buffer, size_t len) {
            return ::recv(sockfd, buffer, len, 0);
        }
};

// Example usage
int main() {
    TCPClient client;

    // Connect with TCP_NODELAY enabled
    if (!client.connect("127.0.0.1", 8080, true)) {
        return 1;
    }

    // Send small messages (won't be delayed by Nagle)
    const char* msg1 = "Hello";
    const char* msg2 = "World";

    client.send(msg1, strlen(msg1));
    client.send(msg2, strlen(msg2));

    return 0;
}
