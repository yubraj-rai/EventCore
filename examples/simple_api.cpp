#include "eventcore/server/server.h"
#include "eventcore/core/logger.h"
#include <iostream>
#include <csignal>

std::atomic<bool> running{true};

void signal_handler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        eventcore::server::Config config;
        config.port = 8080;
        config.num_workers = 2;
        config.num_threads_per_worker = 2;

        eventcore::server::Server server(config);

        server.router().get("/api/users", [](const eventcore::http::Request& req) {
                eventcore::http::Response resp;
                resp.set_status(200);
                resp.set_content_type("application/json");
                resp.set_body(R"([
                {"id": 1, "name": "Alice"},
                {"id": 2, "name": "Bob"},
                {"id": 3, "name": "Charlie"}
            ])");
                return resp;
                });

        server.router().get("/api/users/{id}", [](const eventcore::http::Request& req) {
                std::string path = req.path();
                size_t last_slash = path.find_last_of('/');
                std::string id = path.substr(last_slash + 1);

                eventcore::http::Response resp;
                resp.set_status(200);
                resp.set_content_type("application/json");
                resp.set_body(R"({"id": )" + id + R"(, "name": "User )" + id + "\"}");
                return resp;
                });

        server.router().post("/api/users", [](const eventcore::http::Request& req) {
                eventcore::http::Response resp;
                resp.set_status(201);
                resp.set_content_type("application/json");
                resp.set_body(R"({"status": "created", "data": )" + req.body() + "}");
                return resp;
                });

        std::cout << "Starting example server on port 8080..." << std::endl;
        std::cout << "Try these endpoints:" << std::endl;
        std::cout << "  GET  http://localhost:8080/api/users" << std::endl;
        std::cout << "  GET  http://localhost:8080/api/users/123" << std::endl;
        std::cout << "  POST http://localhost:8080/api/users" << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;

        server.start();

        while (running) std::this_thread::sleep_for(std::chrono::seconds(1));

        server.stop();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
