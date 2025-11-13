#pragma once
#include "../core/noncopyable.h"
#include "../net/socket.h"
#include "../net/buffer.h"
#include "parser.h"
#include "request.h"
#include "response.h"
#include <memory>
#include <functional>
#include <chrono>

namespace eventcore {
    namespace http {

        class Connection : public NonCopyable, public std::enable_shared_from_this<Connection> {
            public:
                using ConnectionPtr = std::shared_ptr<Connection>;
                using RequestHandler = std::function<Response(const Request&)>;
                using CloseCallback = std::function<void(ConnectionPtr)>;

                Connection(net::Socket socket, RequestHandler handler);
                ~Connection();
                void reset(int fd);
                void update_activity();
                bool is_idle(std::chrono::seconds timeout) const;
                void start();
                void handle_read();
                void handle_write();
                void send(const Response& response);
                void shutdown();
                void force_close();
                void set_close_callback(CloseCallback cb) { close_callback_ = cb; }
                bool is_connected() const { return state_ == kConnected; }
                int fd() const { return socket_.fd(); }

            private:
                enum State { kConnecting, kConnected, kDisconnecting, kDisconnected };
                void handle_error();
                void handle_close();
                void process_request();
                void send_response(const Response& response);

                std::chrono::steady_clock::time_point last_activity_;
                net::Socket socket_;
                State state_;
                net::Buffer read_buffer_;
                net::Buffer write_buffer_;
                Parser parser_;
                Request request_;
                RequestHandler request_handler_;
                CloseCallback close_callback_;
        };

        using ConnectionPtr = std::shared_ptr<Connection>;

    } // namespace http
} // namespace eventcore
