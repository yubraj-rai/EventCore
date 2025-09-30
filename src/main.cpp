#include "eventcore/server/server.h"
#include "eventcore/core/logger.h"
#include <csignal>
#include <iostream>
#include <ctime>
#include <sys/resource.h>

std::atomic<bool> running{true};

void signal_handler(int signal) {
    LOG_INFO("Received signal ", signal, ", shutting down...");
    running = false;
}


/*
    Author: Yubraj Rai

    Adjusts the maximum number of file descriptors a process is allowed to use.
    High-performance servers require a large number of simultaneous connections.

    • Each client connection → 1 socket → 1 file descriptor (FD)
    • Default Linux FD limits are typically 1024 or 4096, which is too low
      for servers handling millions of requests.
    • Raising the limit to millions allows the server to handle a massive
      number of concurrent connections without hitting OS restrictions.
*/

void tune_system_limits() {
    struct rlimit limit;

    // Set file descriptor limit
    limit.rlim_cur = 1000000;
    limit.rlim_max = 1000000;

    if (setrlimit(RLIMIT_NOFILE, &limit) == 0) {
        LOG_INFO("Set RLIMIT_NOFILE to ", limit.rlim_cur);
    } else {
        LOG_WARN("Failed to set RLIMIT_NOFILE");
    }

    // Log current limits
    getrlimit(RLIMIT_NOFILE, &limit);
    LOG_INFO("Current FD limit: ", limit.rlim_cur, 
            " (soft) / ", limit.rlim_max, " (hard)");
}

int main() {
    tune_system_limits();
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        eventcore::server::Config config;
        config.port = 8080;
        config.num_workers = 2;

        eventcore::server::Server server(config);

        server.router().get("/", [](const eventcore::http::Request& req) {
                eventcore::http::Response resp;
                resp.set_status(200);
                resp.set_content_type("text/html");
                resp.set_body(R"(
                <html>
                <head><title>EventCore Server</title></head>
                <body>
                    <h1>Welcome to EventCore</h1>
                    <p>High-performance C++ HTTP Server</p>
                    <ul>
                        <li><a href="/health">Health Check</a></li>
                        <li><a href="/api/hello">Hello API</a></li>
                        <li><a href="/api/time">Current Time</a></li>
                        <li><a href="/api/echo">Echo Test</a></li>
                    </ul>
                </body>
                </html>
                )");
                return resp;
        });

        server.router().get("/health", [](const eventcore::http::Request& req) {
                eventcore::http::Response resp = eventcore::http::Response::make_json(200, 
                        R"({"status": "healthy", "server": "EventCore", "timestamp": )" + 
                        std::to_string(std::time(nullptr)) + "}");
                return resp;
                });

        server.router().get("/api/hello", [](const eventcore::http::Request& req) {
                eventcore::http::Response resp = eventcore::http::Response::make_json(200,
                        R"({"message": "Hello from EventCore!", "timestamp": )" + 
                        std::to_string(std::time(nullptr)) + "}");
                return resp;
                });

        server.router().get("/api/time", [](const eventcore::http::Request& req) {
                eventcore::http::Response resp = eventcore::http::Response::make_json(200,
                        R"({"timestamp": )" + std::to_string(std::time(nullptr)) + "}");
                return resp;
                });

        server.router().post("/api/echo", [](const eventcore::http::Request& req) {
                eventcore::http::Response resp = eventcore::http::Response::make_json(200,
                        R"({"echo": ")" + req.body() + "\"}");
                return resp;
                });

        server.router().set_not_found_handler([](const eventcore::http::Request& req) {
                eventcore::http::Response resp;
                resp.set_status(404);
                resp.set_content_type("application/json");
                resp.set_body(R"({"error": "Not Found", "path": ")" + req.path() + "\"}");
                return resp;
                });

        LOG_INFO("Starting EventCore server...");
        server.start();

        while (running) std::this_thread::sleep_for(std::chrono::seconds(1));

        LOG_INFO("Shutting down server...");
        server.stop();

    } catch (const std::exception& e) {
        LOG_ERROR("Server error: ", e.what());
        return 1;
    }

    LOG_INFO("Server stopped successfully");
    return 0;
}
