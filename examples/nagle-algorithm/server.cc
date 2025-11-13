#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

class TCPServer {
    private:
        int server_fd;
        int port;

    public:
        TCPServer(int port) : server_fd(-1), port(port) {}

        ~TCPServer() {
            if (server_fd >= 0) {
                close(server_fd);
            }
        }

        bool start(bool no_delay = true) {
            // Create socket
            server_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (server_fd < 0) {
                std::cerr << "Failed to create socket" << std::endl;
                return false;
            }

            // Set SO_REUSEADDR
            int opt = 1;
            if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, 
                        &opt, sizeof(opt)) < 0) {
                std::cerr << "Failed to set SO_REUSEADDR" << std::endl;
                close(server_fd);
                return false;
            }

            // Bind to port
            struct sockaddr_in address;
            memset(&address, 0, sizeof(address));
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons(port);

            if (bind(server_fd, (struct sockaddr*)&address, 
                        sizeof(address)) < 0) {
                std::cerr << "Bind failed" << std::endl;
                close(server_fd);
                return false;
            }

            // Listen
            if (listen(server_fd, 10) < 0) {
                std::cerr << "Listen failed" << std::endl;
                close(server_fd);
                return false;
            }

            std::cout << "Server listening on port " << port << std::endl;
            return true;
        }

        int acceptClient(bool no_delay = true) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int client_fd = accept(server_fd, 
                    (struct sockaddr*)&client_addr, 
                    &client_len);

            if (client_fd < 0) {
                std::cerr << "Accept failed" << std::endl;
                return -1;
            }

            // Set TCP_NODELAY on client socket
            if (no_delay) {
                int flag = 1;
                if (setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, 
                            &flag, sizeof(int)) < 0) {
                    std::cerr << "Failed to set TCP_NODELAY on client" 
                        << std::endl;
                    close(client_fd);
                    return -1;
                }
            }

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, 
                    client_ip, INET_ADDRSTRLEN);

            std::cout << "Client connected from " << client_ip 
                << ":" << ntohs(client_addr.sin_port) << std::endl;
            std::cout << "TCP_NODELAY: " << (no_delay ? "ON" : "OFF") 
                << std::endl;

            return client_fd;
        }
};

// Example usage
int main() {
    TCPServer server(8080);

    if (!server.start(true)) {  // Enable TCP_NODELAY
        return 1;
    }

    while (true) {
        int client_fd = server.acceptClient(true);
        if (client_fd < 0) {
            continue;
        }

        // Handle client...
        char buffer[1024];
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);
        printf("%s\n", buffer) ;
        if (bytes > 0) {
            send(client_fd, buffer, bytes, 0);  // Echo back
        }

        close(client_fd);
    }

    return 0;
}
