#include "eventcore/core/logger.h"
#include <iostream>
#include <ctime>

namespace eventcore {

    Logger& Logger::instance() {
        static Logger logger;
        return logger;
    }

    Logger::Logger() = default;

    void Logger::set_level(LogLevel level) {
        level_ = level;
    }

    void Logger::set_file(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!filename.empty()) {
            file_ = std::make_unique<std::ofstream>(filename, std::ios::app);
        }
    }

    void Logger::debug(const std::string& msg) {
        write(LogLevel::LEVEL_DEBUG, msg);
    }

    void Logger::info(const std::string& msg) {
        write(LogLevel::LEVEL_INFO, msg);
    }

    void Logger::warn(const std::string& msg) {
        write(LogLevel::LEVEL_WARN, msg);
    }

    void Logger::error(const std::string& msg) {
        write(LogLevel::LEVEL_ERROR, msg);
    }

    void Logger::write(LogLevel level, const std::string& msg) {
        if (level < level_) return;

        std::lock_guard<std::mutex> lock(mutex_);
        std::string log_line = format_time() + " [" + level_string(level) + "] " + msg;

        if (file_ && file_->is_open()) {
            *file_ << log_line << std::endl;
        } else {
            std::cout << log_line << std::endl;
        }
    }

    std::string Logger::format_time() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        char buffer[20];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t));
        return buffer;
    }

    const char* Logger::level_string(LogLevel level) {
        switch (level) {
            case LogLevel::LEVEL_DEBUG: return "DEBUG";
            case LogLevel::LEVEL_INFO:  return "INFO";
            case LogLevel::LEVEL_WARN:  return "WARN";
            case LogLevel::LEVEL_ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }

} // namespace eventcore
