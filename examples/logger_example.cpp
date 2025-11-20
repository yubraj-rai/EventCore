#include "eventcore/core/logger.h"
#include <thread>
#include <chrono>
#include <vector>
#include <iostream>

using namespace eventcore::logging;

void logger_example() {
    // Configure logger with 5MB rollover and timestamped filenames
    LogConfig config;
    config.log_directory = EVENTCORE_LOG_DIR; // From CMake
    config.log_prefix = "eventserver";
    config.min_level = LogLevel::DEBUG;
    config.max_file_size_mb = 5;           // 5MB rollover
    //config.max_backup_files = 5;           // Keep 5 backup files
    config.use_timestamp_suffix = true;    // Add timestamps to filenames
    config.console_output = true;          // Show WARN/ERROR on console
    config.immediate_flush = false;        // Better performance
    //config.batch_writes = true;            // Buffer writes
    //config.batch_size = 10;                // Flush after 10 messages

    if (!Logger::instance().initialize(config)) {
        std::cerr << "Failed to initialize logger!" << std::endl;
        return;
    }

    LOG_INFO("EventCore Server starting...");
    LOG_INFO("Logger configured with 5MB rollover and timestamped filenames");

    // Simulate different log levels
    LOG_DEBUG("Debug message - usually disabled in production");
    LOG_INFO("Server initialized on port 8080");
    LOG_WARN("High memory usage detected: 85%");
    LOG_ERROR("Database connection failed - retrying...");

    // Simulate high-volume logging to trigger rotation
    for (int i = 0; i < 1000; ++i) {
        LOG_INFO("Processing request #", i, " from client");

        if (i % 100 == 0) {
            LOG_FLUSH(); // Explicit flush every 100 messages
        }
    }

    // Multi-threaded logging test
    std::vector<std::thread> threads;
    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([t]() {
                for (int i = 0; i < 100; ++i) {
                LOG_INFO("Thread ", t, " - Message ", i);
                }
                });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    LOG_INFO("Server shutting down gracefully");
    Logger::instance().shutdown();
}

int main() {
    logger_example();
    return 0;
}
