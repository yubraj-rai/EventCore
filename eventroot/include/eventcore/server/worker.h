#pragma once
#include "../core/noncopyable.h"
#include "../net/poller.h"
#include "../http/connection.h"
#include "../http/router.h"
#include "../thread/thread_pool.h"
#include "connection_pool.h"
#include <memory>
#include <unordered_map>
#include <atomic>

namespace eventcore {
    namespace server {

        class Worker : public NonCopyable {
            public:
                Worker(const http::Router* router, size_t thread_pool_size = 4, ConnectionPool* pool = nullptr);
                ~Worker();
                void start();
                void stop();
                void add_connection(http::ConnectionPtr conn);
                size_t connection_count() const { return connections_.size(); }
                bool is_running() const { return running_; }

            private:
                void event_loop();
                void handle_connection_event(int fd, int events);
                void remove_connection(int fd);
                void check_idle_connections();

                ConnectionPool* pool_;
                std::chrono::steady_clock::time_point last_timeout_check_;
                const http::Router* router_;
                std::unique_ptr<net::Poller> poller_;
                std::unique_ptr<thread::ThreadPool> thread_pool_;
                std::unordered_map<int, http::ConnectionPtr> connections_;
                std::thread event_thread_;
                std::atomic<bool> running_{false};
                mutable std::mutex mutex_;
        };

    } // namespace server
} // namespace eventcore
