#include "eventcore/core/logger.h"
#include <iostream>
#include <system_error>

namespace eventcore {
    namespace logging {

        LoggerImpl::LoggerImpl() {
        }

        LoggerImpl::~LoggerImpl() {
            shutdown();
        }

        bool LoggerImpl::initialize(const LogConfig& config) {
            if (initialized_.load(std::memory_order_acquire)) {
                return true;
            }

            // Validate configuration
            if (config.log_directory.empty()) {
                std::cerr << "Log directory cannot be empty" << std::endl;
                return false;
            }

            // Copy configuration
            config_.log_directory = config.log_directory;
            config_.log_prefix = config.log_prefix;
            config_.max_file_size_mb = config.max_file_size_mb;
            config_.use_timestamp_suffix = config.use_timestamp_suffix;
            config_.console_output = config.console_output;
            config_.immediate_flush = config.immediate_flush;
            config_.min_level.store(config.min_level, std::memory_order_relaxed);

            // Create log directory
            if (!create_log_directory()) {
                std::cerr << "Failed to create log directory: " << config_.log_directory << std::endl;
                return false;
            }

            // Open log file
            current_log_file_ = get_log_filename();

            log_file_ = std::make_unique<std::ofstream>(
                    current_log_file_, std::ios::app | std::ios::out
                    );

            if (!log_file_->is_open()) {
                std::cerr << "Failed to open log file: " << current_log_file_ << std::endl;
                return false;
            }

            // Get current file size
            struct stat st;
            if (stat(current_log_file_.c_str(), &st) == 0) {
                current_file_size_.store(st.st_size, std::memory_order_relaxed);
            }

            initialized_.store(true, std::memory_order_release);

            // Log initialization
            log(LogLevel::INFO, "EventCore Logger initialized", __FILE__, __LINE__);

            return true;
        }

        void LoggerImpl::shutdown() {
            if (!initialized_.load(std::memory_order_acquire)) {
                return;
            }

            log(LogLevel::INFO, "EventCore Logger shutting down", __FILE__, __LINE__);

            std::lock_guard<std::mutex> lock(file_mutex_);
            if (log_file_ && log_file_->is_open()) {
                log_file_->flush();
                log_file_->close();
            }

            initialized_.store(false, std::memory_order_release);
        }

        void LoggerImpl::log(LogLevel level, const std::string& message,
                const char* file, int line) {
            if (!initialized_.load(std::memory_order_acquire) ||
                    level < config_.min_level.load(std::memory_order_relaxed)) {
                return;
            }

            std::string formatted = format_log_entry(level, message, file, line);

            // Write directly to file
            write_to_file(formatted);

            // Also output to console for WARN and ERROR
            if (config_.console_output && level >= LogLevel::WARN) {
                std::cerr << formatted << std::flush;
            }
        }

        void LoggerImpl::flush() {
            std::lock_guard<std::mutex> lock(file_mutex_);
            if (log_file_ && log_file_->is_open()) {
                log_file_->flush();
            }
        }

        void LoggerImpl::write_to_file(const std::string& formatted_message) {
            std::lock_guard<std::mutex> lock(file_mutex_);

            if (!log_file_ || !log_file_->is_open()) {
                return; // Should never happen if initialized properly
            }

            // Check if we need to rotate
            size_t message_size = formatted_message.size();
            size_t new_size = current_file_size_.load(std::memory_order_relaxed) + message_size;
            size_t max_size = config_.max_file_size_mb * 1024 * 1024;

            if (new_size >= max_size) {
                perform_log_rotation();
                new_size = message_size;
            }

            // Write to file
            *log_file_ << formatted_message;

            if (config_.immediate_flush) {
                log_file_->flush();
            }

            current_file_size_.store(new_size, std::memory_order_relaxed);
        }

        void LoggerImpl::perform_log_rotation() {
            if (log_file_ && log_file_->is_open()) {
                log_file_->flush();
                log_file_->close();
            }

            // Simple rotation: just create new timestamped file
            current_log_file_ = get_log_filename();

            log_file_ = std::make_unique<std::ofstream>(
                    current_log_file_, std::ios::app | std::ios::out
                    );

            current_file_size_.store(0, std::memory_order_relaxed);
        }

        std::string LoggerImpl::format_log_entry(LogLevel level, const std::string& message,
                const char* file, int line) {
            std::ostringstream oss;

            oss << "[" << get_timestamp() << "] "
                << "[" << get_level_string(level) << "] "
                << "[" << extract_filename(file) << ":" << line << "] "
                << message << "\n";

            return oss.str();
        }

        std::string LoggerImpl::get_timestamp() {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()
                    ) % 1000;

            std::tm tm_buf;
            localtime_r(&time_t, &tm_buf);

            char buffer[32];
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_buf);

            std::ostringstream oss;
            oss << buffer << "." << std::setfill('0') << std::setw(3) << ms.count();
            return oss.str();
        }

        std::string LoggerImpl::get_level_string(LogLevel level) {
            switch (level) {
                case LogLevel::DEBUG: return "DEBUG";
                case LogLevel::INFO:  return "INFO ";
                case LogLevel::WARN:  return "WARN ";
                case LogLevel::ERROR: return "ERROR";
                default: return "UNKNOWN";
            }
        }

        std::string LoggerImpl::extract_filename(const char* path) {
            std::string p(path);
            size_t pos = p.find_last_of("/\\");
            return pos != std::string::npos ? p.substr(pos + 1) : p;
        }

        std::string LoggerImpl::get_log_filename() {
            if (config_.use_timestamp_suffix) {
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);
                std::tm tm_buf;
                localtime_r(&time_t, &tm_buf);

                char time_buf[20];
                strftime(time_buf, sizeof(time_buf), "%Y%m%d_%H%M%S", &tm_buf);

                return config_.log_directory + "/" + config_.log_prefix + "_" + time_buf + ".log";
            } else {
                return config_.log_directory + "/" + config_.log_prefix + ".log";
            }
        }

        bool LoggerImpl::create_log_directory() {
            struct stat st;
            if (stat(config_.log_directory.c_str(), &st) == 0) {
                return S_ISDIR(st.st_mode);
            }

            return mkdir(config_.log_directory.c_str(), 0755) == 0 || errno == EEXIST;
        }

    } // namespace logging
} // namespace eventcore
