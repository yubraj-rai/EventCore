#pragma once

#include <string>
#include <cstdint>

namespace eventcore {
    namespace server {

        struct Config {
            std::string host = "0.0.0.0";
            uint16_t port = 8080;
            int backlog = 4096;  // Increased from 1024

            size_t num_workers = 0;
            size_t num_threads_per_worker = 4;

            size_t max_connections = 100000;  // Increased from 10000
            size_t connection_pool_size = 100000;  // NEW

            size_t max_request_size = 1024 * 1024;
            int keepalive_timeout_sec = 60;

            size_t read_buffer_size = 4096;
            size_t write_buffer_size = 4096;

            bool tcp_nodelay = true;
            bool tcp_reuseaddr = true;
            bool tcp_reuseport = true;  // Changed from false

            int accept_batch_size = 100;  // NEW

            std::string log_file;
            std::string log_level = "info";
        };

    } // namespace server
} // namespace eventcore

