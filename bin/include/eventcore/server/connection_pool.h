#pragma once

#include "../http/connection.h"
#include "../core/noncopyable.h"
#include <vector>
#include <mutex>
#include <unordered_map>
#include <chrono>

namespace eventcore {
    namespace server {

        class ConnectionPool : public NonCopyable {
            public:
                explicit ConnectionPool(size_t size);
                ~ConnectionPool() = default;

                http::ConnectionPtr acquire(int fd, http::Connection::RequestHandler handler);
                void release(int fd);

                size_t available() const;
                size_t total_size() const { return pool_.size(); }

                // Timeout management
                std::vector<int> get_idle_connections(std::chrono::seconds timeout);

            private:
                struct PoolEntry {
                    http::ConnectionPtr conn;
                    std::chrono::steady_clock::time_point last_used;
                    bool in_use;
                };

                std::vector<PoolEntry> pool_;
                std::vector<size_t> free_indices_;
                std::unordered_map<int, size_t> fd_to_index_;
                mutable std::mutex mutex_;
        };

    } // namespace server
} // namespace eventcore
