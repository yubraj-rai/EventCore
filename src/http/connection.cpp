#include "eventcore/http/connection.h"
#include "eventcore/core/logger.h"
#include <unistd.h>
#include <sstream>

namespace eventcore {
    namespace http {

        Connection::Connection(net::Socket socket, RequestHandler handler)
            : socket_(std::move(socket)), state_(kConnecting), request_handler_(handler) {
                auto result = socket_.set_nonblocking(true);
                if (result.is_err()) {
                    std::stringstream ss;
                    ss << "Failed to set socket non-blocking: " << result.error();
                    LOG_ERROR(ss.str());
                }
            }

        Connection::~Connection() { if (state_ != kDisconnected) force_close(); }

        void Connection::start() { state_ = kConnected; handle_read(); }

        void Connection::send(const Response& response) {
            if (state_ != kConnected) return;
            std::string response_str = response.to_string();
            write_buffer_.append(response_str.data(), response_str.size());
            handle_write();
        }

        void Connection::shutdown() { if (state_ == kConnected) state_ = kDisconnecting; socket_.shutdown_write(); }

        void Connection::force_close() {
            if (state_ != kDisconnected) {
                state_ = kDisconnected; socket_.close();
                if (close_callback_) close_callback_(shared_from_this());
            }
        }

        void Connection::handle_read() {
            if (state_ != kConnected) return;

            // Edge-triggered: read until EAGAIN
            while (true) {
                ssize_t n = read_buffer_.read_from_fd(socket_.fd());

                if (n > 0) {
                    update_activity();
                    process_request();
                } else if (n == 0) {
                    handle_close();
                    break;
                } else {
                    // Check errno
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;  // No more data available
                    }
                    handle_error();
                    break;
                }
            }
        }


        void Connection::handle_write() {
            if (state_ != kConnected && state_ != kDisconnecting) return;
            if (write_buffer_.readable_bytes() > 0) {
                auto result = socket_.send(write_buffer_.peek(), write_buffer_.readable_bytes());
                if (result.is_ok()) {
                    write_buffer_.retrieve(result.value());
                    if (write_buffer_.readable_bytes() == 0 && state_ == kDisconnecting) force_close();
                } else handle_error();
            } else if (state_ == kDisconnecting) force_close();
        }

        void Connection::handle_error() { 
            std::stringstream ss;
            ss << "Connection error on fd: " << socket_.fd();
            LOG_ERROR(ss.str()); 
            force_close(); 
        }

        void Connection::handle_close() { 
            std::stringstream ss;
            ss << "Connection closed on fd: " << socket_.fd();
            LOG_DEBUG(ss.str()); 
            force_close(); 
        }

        void Connection::process_request() {
            while (true) {
                Request request; parser_.reset();
                if (!parser_.parse_request(&read_buffer_, &request)) break;
                if (parser_.is_complete()) {
                    Response response = request_handler_(request); send_response(response);
                    std::string connection = request.get_header("Connection");
                    if (connection == "close") { shutdown(); break; }
                } else break;
            }
        }

        void Connection::send_response(const Response& response) {
            std::string request_connection = request_.get_header("Connection");
            bool keep_alive = request_connection == "keep-alive" || (request_.version() == Version::HTTP_1_1 && request_connection != "close");
            Response resp = response; resp.set_keep_alive(keep_alive); send(resp);
        }

        void Connection::reset(int fd) {
            socket_ = net::Socket(fd);
            auto result = socket_.set_nonblocking(true);
            if (result.is_err()) {
                LOG_ERROR("Failed to set non-blocking: ", result.error());
            }

            state_ = kConnecting;
            read_buffer_.retrieve_all();
            write_buffer_.retrieve_all();
            parser_.reset();
            request_.reset();
            last_activity_ = std::chrono::steady_clock::now();
        }

        void Connection::update_activity() {
            last_activity_ = std::chrono::steady_clock::now();
        }

        bool Connection::is_idle(std::chrono::seconds timeout) const {
            auto now = std::chrono::steady_clock::now();
            return (now - last_activity_) > timeout;
        }


    } // namespace http
} // namespace eventcore
