#include "eventcore/server/worker.h"
#include "eventcore/core/logger.h"
#include <unistd.h>
#include <cstring>
#include <sstream>

namespace eventcore {
    namespace server {
        // Worker::Worker(const http::Router* router, 
        //         size_t thread_pool_size,
        //         ConnectionPool* pool)
        //     : router_(router), 
        //     pool_(pool),
        //     thread_pool_(std::make_unique<thread::ThreadPool>(thread_pool_size)),
        //     last_timeout_check_(std::chrono::steady_clock::now()) {

        //         poller_ = net::Poller::create();
        //         if (!poller_) throw std::runtime_error("Failed to create poller");
        //     }

        Worker::Worker(const http::Router* router, size_t thread_pool_size, ConnectionPool* pool)
            : router_(router),
            pool_(pool),
            thread_pool_(std::make_unique<thread::ThreadPool>(thread_pool_size)),
            last_timeout_check_(std::chrono::steady_clock::now()) {

                poller_ = net::Poller::create();
                if (!poller_) throw std::runtime_error("Failed to create poller");
            }

        Worker::~Worker() { stop(); }

        void Worker::start() {
            if (running_) return;
            running_ = true; 
            thread_pool_->start();
            event_thread_ = std::thread(&Worker::event_loop, this);
            LOG_INFO("Worker started");
        }

        void Worker::stop() {
            if (!running_) return;
            running_ = false;
            if (event_thread_.joinable()) event_thread_.join();
            thread_pool_->stop();
            { 
                std::lock_guard<std::mutex> lock(mutex_); 
                connections_.clear(); 
            }
            LOG_INFO("Worker stopped");
        }
        void Worker::add_connection(http::ConnectionPtr conn) {
            std::lock_guard<std::mutex> lock(mutex_);
            int fd = conn->fd();
            connections_[fd] = conn;

            conn->set_close_callback([this](http::ConnectionPtr conn) {
                    remove_connection(conn->fd());
                    pool_->release(conn->fd());
                    });

            if (!poller_->add(fd, net::Poller::kReadable, 
                        [this](int fd, int events) {
                        handle_connection_event(fd, events);
                        })) {
                LOG_ERROR("Failed to add connection to poller");
                connections_.erase(fd);
                pool_->release(fd);
                return;
            }

            conn->start();
        }

        void Worker::check_idle_connections() {
            auto now = std::chrono::steady_clock::now();

            // Check every 5 seconds
            if (now - last_timeout_check_ < std::chrono::seconds(5)) {
                return;
            }

            last_timeout_check_ = now;

            auto idle_fds = pool_->get_idle_connections(std::chrono::seconds(60));

            for (int fd : idle_fds) {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = connections_.find(fd);
                if (it != connections_.end()) {
                    LOG_DEBUG("Closing idle connection: ", fd);
                    poller_->remove(fd);
                    close(fd);
                    connections_.erase(it);
                    pool_->release(fd);
                }
            }
        }

        void Worker::event_loop() {
            while (running_) {
                try {
                    int num_events = poller_->poll(100);

                    if (num_events < 0 && errno != EINTR) {
                        LOG_ERROR("Poller error");
                        break;
                    }

                    // Check for idle connections periodically
                    check_idle_connections();

                } catch (const std::exception& e) {
                    LOG_ERROR("Exception in worker event loop: ", e.what());
                }
            }
        }
        void Worker::handle_connection_event(int fd, int events) {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = connections_.find(fd);
            if (it == connections_.end()) return;

            auto conn = it->second;

            try {
                if (events & net::Poller::kError) {
                    remove_connection(fd);
                    pool_->release(fd);
                    return;
                }

                if (events & net::Poller::kReadable) {
                    // Process in thread pool
                    thread_pool_->submit([conn, this, fd]() {
                            conn->handle_read();
                            conn->update_activity();
                            });
                }

                if (events & net::Poller::kWritable) {
                    thread_pool_->submit([conn]() {
                            conn->handle_write();
                            });
                }

            } catch (const std::exception& e) {
                LOG_ERROR("Error handling connection event: ", e.what());
                remove_connection(fd);
                pool_->release(fd);
            }
        }
        void Worker::remove_connection(int fd) {
            poller_->remove(fd); 
            connections_.erase(fd);
        }

    } // namespace server
} // namespace eventcore
