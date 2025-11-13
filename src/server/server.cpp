#include "eventcore/server/server.h"
#include "eventcore/core/logger.h"
#include <algorithm>
#include <sstream>

namespace eventcore {
    namespace server {

        Server::Server(const Config& config)
            : config_(config),
            pool_(config_.connection_pool_size) {  // Initialize pool

                if (config_.num_workers == 0) {
                    config_.num_workers = std::thread::hardware_concurrency();
                    if (config_.num_workers == 0) config_.num_workers = 1;
                }

                for (size_t i = 0; i < config_.num_workers; ++i) {
                    workers_.push_back(std::make_unique<Worker>(
                                &router_,
                                config_.num_threads_per_worker,
                                &pool_));  // Pass pool pointer
                }

                LOG_INFO("Server configured with ", config_.num_workers, " workers, ",
                        config_.num_threads_per_worker, " threads each, ",
                        config_.connection_pool_size, " connection pool");
            }

        Server::~Server() {
            stop();
        }

        void Server::start() {
            if (running_) return;

            auto result = net::Socket::create_tcp();
            if (result.is_err())
                throw std::runtime_error("Failed to create socket: " + result.error());
            listen_socket_ = std::move(result.value());

            // Set socket options
            auto reuseaddr_result = listen_socket_.set_reuseaddr(true);
            if (reuseaddr_result.is_err())
                throw std::runtime_error("Set reuseaddr failed: " + reuseaddr_result.error());

            auto reuseport_result = listen_socket_.set_reuseport(config_.tcp_reuseport);
            if (reuseport_result.is_err())
                throw std::runtime_error("Set reuseport failed: " + reuseport_result.error());

            auto nodelay_result = listen_socket_.set_nodelay(config_.tcp_nodelay);
            if (nodelay_result.is_err())
                throw std::runtime_error("Set nodelay failed: " + nodelay_result.error());

            auto keepalive_result = listen_socket_.set_keepalive(true);
            if (keepalive_result.is_err())
                throw std::runtime_error("Set keepalive failed: " + keepalive_result.error());

            // Bind and listen
            net::Address addr(config_.host, config_.port);
            auto bind_result = listen_socket_.bind(addr);
            if (bind_result.is_err())
                throw std::runtime_error("Bind failed: " + bind_result.error());

            auto listen_result = listen_socket_.listen(config_.backlog);
            if (listen_result.is_err())
                throw std::runtime_error("Listen failed: " + listen_result.error());

            auto nonblock_result = listen_socket_.set_nonblocking(true);
            if (nonblock_result.is_err())
                throw std::runtime_error("Set non-blocking failed: " + nonblock_result.error());

            // Start workers
            for (auto& worker : workers_) {
                worker->start();
            }

            running_ = true;
            accept_thread_ = std::thread(&Server::accept_loop, this);

            std::stringstream ss;
            ss << "Server started on " << config_.host << ":" << config_.port;
            LOG_INFO(ss.str());
        }

        void Server::stop() {
            if (!running_) return;
            running_ = false;

            if (accept_thread_.joinable()) accept_thread_.join();
            for (auto& worker : workers_) {
                worker->stop();
            }

            listen_socket_.close();
            LOG_INFO("Server stopped");
        }

        void Server::wait() {
            if (accept_thread_.joinable()) {
                accept_thread_.join();
            }
        }

        void Server::accept_loop() {
            while (running_) {
                int accepted = 0;

                for (int i = 0; i < config_.accept_batch_size && running_; ++i) {
                    auto result = listen_socket_.accept();

                    if (result.is_ok()) {
                        net::Socket client_socket = std::move(result.value());
                        handle_new_connection(std::move(client_socket));
                        accepted++;
                    } else {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            LOG_ERROR("Accept error: ", result.error());
                        }
                        break;  // No more pending connections
                    }
                }

                if (accepted == 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            }
        }

        void Server::handle_new_connection(net::Socket client_socket) {
            int fd = client_socket.fd();

            auto request_handler = [this](const http::Request& req) {
                return router_.route(req);
            };

            auto conn = pool_.acquire(fd, request_handler);

            if (!conn) {
                LOG_WARN("Connection pool exhausted, rejecting connection");
                client_socket.close();
                return;
            }

            client_socket.release();

            Worker* worker = next_worker();
            if (worker) {
                worker->add_connection(conn);
            } else {
                LOG_ERROR("No available workers");
                pool_.release(fd);
            }
        }

        Worker* Server::next_worker() {
            size_t idx = next_worker_idx_++ % workers_.size();
            return workers_[idx].get();
        }

    } // namespace server
} // namespace eventcore

