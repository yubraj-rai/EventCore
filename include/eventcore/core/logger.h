#pragma once
#include <string>
#include <memory>
#include <mutex>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace eventcore {

    enum class LogLevel { 
        LEVEL_DEBUG = 0, 
        LEVEL_INFO = 1, 
        LEVEL_WARN = 2, 
        LEVEL_ERROR = 3 
    };

    class Logger {
        public:
            static Logger& instance();
            void set_level(LogLevel level);
            void set_file(const std::string& filename);
            void debug(const std::string& msg);
            void info(const std::string& msg);
            void warn(const std::string& msg);
            void error(const std::string& msg);

        private:
            Logger();
            void write(LogLevel level, const std::string& msg);
            std::string format_time();
            const char* level_string(LogLevel level);

            LogLevel level_ = LogLevel::LEVEL_INFO;
            std::mutex mutex_;
            std::unique_ptr<std::ofstream> file_;
    };

} // namespace eventcore

// Enhanced logging macros with string formatting
#define LOG_DEBUG(...) do { \
    std::stringstream ss; \
    ss << __VA_ARGS__; \
    eventcore::Logger::instance().debug(ss.str()); \
} while(0)

#define LOG_INFO(...) do { \
    std::stringstream ss; \
    ss << __VA_ARGS__; \
    eventcore::Logger::instance().info(ss.str()); \
} while(0)

#define LOG_WARN(...) do { \
    std::stringstream ss; \
    ss << __VA_ARGS__; \
    eventcore::Logger::instance().warn(ss.str()); \
} while(0)

#define LOG_ERROR(...) do { \
    std::stringstream ss; \
    ss << __VA_ARGS__; \
    eventcore::Logger::instance().error(ss.str()); \
} while(0)
