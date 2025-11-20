#include "eventcore/server/server.h"
#include "eventcore/core/logger.h"
#include <csignal>
#include <cstring>
#include <iostream>
#include <ctime>
#include <sys/resource.h>
#include <cstdlib>

std::atomic<bool> running{true};

void signal_handler(int signal) {
    LOG_INFO("Received signal ", signal, " (", strsignal(signal), "), shutting down...");
    running = false;
}

bool setup_signal_handlers() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        std::cerr << "Failed to set SIGINT handler: " << strerror(errno) << std::endl;
        return false;
    }

    if (sigaction(SIGTERM, &sa, nullptr) == -1) {
        std::cerr << "Failed to set SIGTERM handler: " << strerror(errno) << std::endl;
        return false;
    }

    // Ignore SIGPIPE to prevent crashes on broken pipes
    signal(SIGPIPE, SIG_IGN);

    return true;
}

void tune_system_limits() {
    struct rlimit limit;

    // Get current limits first
    if (getrlimit(RLIMIT_NOFILE, &limit) == 0) {
        LOG_INFO("Current FD limit: ", limit.rlim_cur, " (soft) / ", limit.rlim_max, " (hard)");
    }

    // Set file descriptor limit
    limit.rlim_cur = 1000000;
    limit.rlim_max = 1000000;

    if (setrlimit(RLIMIT_NOFILE, &limit) == 0) {
        LOG_INFO("Set RLIMIT_NOFILE to ", limit.rlim_cur);
    } else {
        LOG_WARN("Failed to set RLIMIT_NOFILE: ", strerror(errno));
        // Continue with current limits
    }

    // Verify new limits
    getrlimit(RLIMIT_NOFILE, &limit);
    LOG_INFO("New FD limit: ", limit.rlim_cur, " (soft) / ", limit.rlim_max, " (hard)");
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n"
        << "Options:\n"
        << "  -p, --port PORT          Server port (default: 8080)\n"
        << "  -w, --workers NUM        Number of worker threads (default: auto)\n"
        << "  -h, --help               Show this help message\n"
        << "  -v, --verbose            Enable verbose logging\n"
        << std::endl;
}

eventcore::server::Config parse_command_line(int argc, char* argv[]) {
    eventcore::server::Config config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            exit(0);
        }
        else if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) {
                config.port = static_cast<uint16_t>(std::stoi(argv[++i]));
            } else {
                std::cerr << "Error: --port requires an argument" << std::endl;
                exit(1);
            }
        }
        else if (arg == "-w" || arg == "--workers") {
            if (i + 1 < argc) {
                config.num_workers = std::stoul(argv[++i]);
            } else {
                std::cerr << "Error: --workers requires an argument" << std::endl;
                exit(1);
            }
        }
        else if (arg == "-v" || arg == "--verbose") {
            // Verbose mode already handled by log level
        }
        else {
            std::cerr << "Error: Unknown option " << arg << std::endl;
            print_usage(argv[0]);
            exit(1);
        }
    }

    return config;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments first
    eventcore::server::Config config = parse_command_line(argc, argv);

    // Initialize logger
    eventcore::logging::LogConfig log_config;
    log_config.log_directory = EVENTCORE_LOG_DIR;
    log_config.log_prefix = "eventcore_server";
    log_config.min_level = eventcore::logging::LogLevel::DEBUG;
    log_config.max_file_size_mb = 5;
    log_config.use_timestamp_suffix = true;
    log_config.console_output = true;
    log_config.immediate_flush = true;  // Changed to true for reliability

    if (!eventcore::logging::Logger::instance().initialize(log_config)) {
        std::cerr << "Failed to initialize logger - using console only" << std::endl;
    }

    // Setup signal handlers
    if (!setup_signal_handlers()) {
        LOG_ERROR("Failed to setup signal handlers");
        eventcore::logging::Logger::instance().shutdown();
        return 1;
    }

    LOG_INFO("==========================================");
    LOG_INFO("EventCore Server Starting");
    LOG_INFO("==========================================");
    LOG_INFO("Version: 1.0.0");
    LOG_INFO("Log Directory: ", EVENTCORE_LOG_DIR);
    LOG_INFO("Log Level: DEBUG");
    LOG_INFO("File Rollover: 5MB");
    LOG_INFO("Port: ", config.port);
    LOG_INFO("Workers: ", config.num_workers);
    LOG_INFO("PID: ", getpid());
    LOG_INFO("==========================================");

    try {
        // Tune system limits
        tune_system_limits();

        // Create and configure server
        eventcore::server::Server server(config);

        // Setup routes
        server.router().get("/", [](const eventcore::http::Request& req) {
                LOG_DEBUG("Root path accessed from: ", req.get_header("User-Agent"));
                eventcore::http::Response resp;
                resp.set_status(200);
                resp.set_content_type("text/html");
                resp.set_body(R"(
<html>
<head>
    <title>EventCore Server</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        h1 { color: #333; }
        ul { list-style-type: none; padding: 0; }
        li { margin: 10px 0; }
        a { text-decoration: none; color: #0066cc; }
        a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <h1>Welcome to EventCore</h1>
    <p>High-performance C++ HTTP Server with Enhanced Logging</p>
    <ul>
        <li><a href="/health">Health Check</a></li>
        <li><a href="/api/hello">Hello API</a></li>
        <li><a href="/api/time">Current Time</a></li>
        <li><a href="/api/echo">Echo Test</a></li>
        <li><a href="/api/status">Server Status</a></li>
    </ul>
</body>
</html>
            )");
                return resp;
        });

        server.router().get("/health", [](const eventcore::http::Request& req) {
                LOG_DEBUG("Health check requested from: ", req.get_header("User-Agent"));
                eventcore::http::Response resp = eventcore::http::Response::make_json(200, 
                        R"({"status": "healthy", "server": "EventCore", "timestamp": )" + 
                        std::to_string(std::time(nullptr)) + "}");
                return resp;
                });

        server.router().get("/api/hello", [](const eventcore::http::Request& req) {
                LOG_INFO("Hello API called from: ", req.get_header("User-Agent"));
                eventcore::http::Response resp = eventcore::http::Response::make_json(200,
                        R"({"message": "Hello from EventCore with Enhanced Logging!", "timestamp": )" + 
                        std::to_string(std::time(nullptr)) + "}");
                return resp;
                });

        server.router().get("/api/time", [](const eventcore::http::Request& req) {
                eventcore::http::Response resp = eventcore::http::Response::make_json(200,
                        R"({"timestamp": )" + std::to_string(std::time(nullptr)) + 
                        R"(, "iso_time": ")" + []() {
                        auto now = std::time(nullptr);
                        char buf[100];
                        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
                        return std::string(buf);
                        }() + "\"}");
                return resp;
                });

        server.router().post("/api/echo", [](const eventcore::http::Request& req) {
                LOG_DEBUG("Echo request with body size: ", req.body().size(), 
                        " from: ", req.get_header("User-Agent"));
                eventcore::http::Response resp = eventcore::http::Response::make_json(200,
                        R"({"echo": ")" + req.body() + "\", \"length\": " + 
                        std::to_string(req.body().size()) + "}");
                return resp;
                });

        server.router().get("/api/status", [](const eventcore::http::Request& req) {
                eventcore::http::Response resp = eventcore::http::Response::make_json(200,
                        R"({"status": "running", "server": "EventCore", "version": "1.0.0", "timestamp": )" + 
                        std::to_string(std::time(nullptr)) + "}");
                return resp;
                });

        server.router().set_not_found_handler([](const eventcore::http::Request& req) {
                LOG_WARN("404 Not Found: ", req.path(), " from ", req.get_header("User-Agent"),
                        " [", eventcore::http::Request::method_to_string(req.method()), "]");
                eventcore::http::Response resp;
                resp.set_status(404);
                resp.set_content_type("application/json");
                resp.set_body(R"({"error": "Not Found", "path": ")" + req.path() +
                        R"(", "method": ")" + eventcore::http::Request::method_to_string(req.method()) + "\"}");
                return resp;
                });

        LOG_INFO("Starting EventCore server on port ", config.port, "...");
        server.start();

        LOG_INFO("Server is running. Press Ctrl+C to stop.");
        LOG_INFO("Available endpoints:");
        LOG_INFO("  GET  http://localhost:", config.port, "/");
        LOG_INFO("  GET  http://localhost:", config.port, "/health");
        LOG_INFO("  GET  http://localhost:", config.port, "/api/hello");
        LOG_INFO("  GET  http://localhost:", config.port, "/api/time");
        LOG_INFO("  POST http://localhost:", config.port, "/api/echo");
        LOG_INFO("  GET  http://localhost:", config.port, "/api/status");

        // Main event loop
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        LOG_INFO("Shutting down server...");
        server.stop();

        } catch (const std::exception& e) {
            LOG_ERROR("Server error: ", e.what());
            eventcore::logging::Logger::instance().shutdown();
            return 1;
        }

        LOG_INFO("Server stopped successfully");
        LOG_INFO("==========================================");

        // Shutdown logger gracefully
        eventcore::logging::Logger::instance().shutdown();

        return 0;
        }
