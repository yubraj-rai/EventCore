#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>
#include <atomic>

namespace eventcore {
    namespace logging {

        enum class LogLevel {
            DEBUG = 0,
            INFO  = 1,
            WARN  = 2,
            ERROR = 3
        };

        struct LogConfig {
            LogLevel min_level = LogLevel::INFO;
            std::string log_directory;
            std::string log_prefix = "eventcore";
            size_t max_file_size_mb = 5;           // 5MB rollover
            bool use_timestamp_suffix = true;      // Add timestamps to filenames
            bool console_output = true;            // Output WARN/ERROR to console
            bool immediate_flush = true;           // Flush after each write
        };

        class LoggerImpl {
            public:
                LoggerImpl();
                ~LoggerImpl();

                bool initialize(const LogConfig& config);
                void shutdown();
                void log(LogLevel level, const std::string& message, const char* file, int line);
                void flush();

                void set_level(LogLevel level) { 
                    config_.min_level.store(level, std::memory_order_relaxed); 
                }
                LogLevel get_level() const { 
                    return config_.min_level.load(std::memory_order_relaxed); 
                }
                bool is_initialized() const { return initialized_.load(std::memory_order_acquire); }

            private:
                void write_to_file(const std::string& formatted_message);
                void perform_log_rotation();

                std::string format_log_entry(LogLevel level, const std::string& message,
                        const char* file, int line);
                std::string get_timestamp();
                std::string get_level_string(LogLevel level);
                std::string extract_filename(const char* path);
                std::string get_log_filename();
                bool create_log_directory();

                struct Config {
                    std::atomic<LogLevel> min_level{LogLevel::INFO};
                    std::string log_directory;
                    std::string log_prefix;
                    size_t max_file_size_mb;
                    bool use_timestamp_suffix;
                    bool console_output;
                    bool immediate_flush;
                } config_;

                std::atomic<bool> initialized_{false};
                std::unique_ptr<std::ofstream> log_file_;
                std::string current_log_file_;
                std::atomic<size_t> current_file_size_{0};
                std::mutex file_mutex_;
        };

        class Logger {
            public:
                static LoggerImpl& instance() {
                    static LoggerImpl instance;
                    return instance;
                }

            private:
                Logger() = delete;
                ~Logger() = delete;
                Logger(const Logger&) = delete;
                Logger& operator=(const Logger&) = delete;
        };

        namespace detail {
            inline void format_args(std::ostringstream&) {}
            template<typename T, typename... Args>
                inline void format_args(std::ostringstream& oss, T&& arg, Args&&... args) {
                    oss << std::forward<T>(arg);
                    format_args(oss, std::forward<Args>(args)...);
                }
        } // namespace detail

#define LOG_DEBUG(...) \
        do { \
            auto& logger = ::eventcore::logging::Logger::instance(); \
            if (logger.is_initialized() && logger.get_level() <= ::eventcore::logging::LogLevel::DEBUG) { \
                std::ostringstream __log_oss; \
                ::eventcore::logging::detail::format_args(__log_oss, __VA_ARGS__); \
                logger.log(::eventcore::logging::LogLevel::DEBUG, __log_oss.str(), __FILE__, __LINE__); \
            } \
        } while(0)

#define LOG_INFO(...) \
        do { \
            auto& logger = ::eventcore::logging::Logger::instance(); \
            if (logger.is_initialized() && logger.get_level() <= ::eventcore::logging::LogLevel::INFO) { \
                std::ostringstream __log_oss; \
                ::eventcore::logging::detail::format_args(__log_oss, __VA_ARGS__); \
                logger.log(::eventcore::logging::LogLevel::INFO, __log_oss.str(), __FILE__, __LINE__); \
            } \
        } while(0)

#define LOG_WARN(...) \
        do { \
            auto& logger = ::eventcore::logging::Logger::instance(); \
            if (logger.is_initialized() && logger.get_level() <= ::eventcore::logging::LogLevel::WARN) { \
                std::ostringstream __log_oss; \
                ::eventcore::logging::detail::format_args(__log_oss, __VA_ARGS__); \
                logger.log(::eventcore::logging::LogLevel::WARN, __log_oss.str(), __FILE__, __LINE__); \
            } \
        } while(0)

#define LOG_ERROR(...) \
        do { \
            auto& logger = ::eventcore::logging::Logger::instance(); \
            if (logger.is_initialized()) { \
                std::ostringstream __log_oss; \
                ::eventcore::logging::detail::format_args(__log_oss, __VA_ARGS__); \
                logger.log(::eventcore::logging::LogLevel::ERROR, __log_oss.str(), __FILE__, __LINE__); \
            } \
        } while(0)

#define LOG_FLUSH() \
        do { \
            if (::eventcore::logging::Logger::instance().is_initialized()) { \
                ::eventcore::logging::Logger::instance().flush(); \
            } \
        } while(0)

    } // namespace logging
} // namespace eventcore
