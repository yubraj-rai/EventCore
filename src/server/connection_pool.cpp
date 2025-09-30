#include "eventcore/server/connection_pool.h"

namespace eventcore {
    namespace server {

        ConnectionPool::ConnectionPool(size_t size) {
            pool_.resize(size);
            free_indices_.reserve(size);

            for (size_t i = 0; i < size; ++i) {
                pool_[i].in_use = false;
                free_indices_.push_back(i);
            }
        }

        http::ConnectionPtr ConnectionPool::acquire(
                int fd, http::Connection::RequestHandler handler) {

            std::lock_guard<std::mutex> lock(mutex_);

            if (free_indices_.empty()) {
                return nullptr;
            }

            size_t idx = free_indices_.back();
            free_indices_.pop_back();

            auto& entry = pool_[idx];

            if (!entry.conn) {
                entry.conn = std::make_shared<http::Connection>(
                        net::Socket(fd), handler);
            } else {
                entry.conn->reset(fd);
            }

            entry.in_use = true;
            entry.last_used = std::chrono::steady_clock::now();
            fd_to_index_[fd] = idx;

            return entry.conn;
        }

        void ConnectionPool::release(int fd) {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = fd_to_index_.find(fd);
            if (it == fd_to_index_.end()) return;

            size_t idx = it->second;
            pool_[idx].in_use = false;
            free_indices_.push_back(idx);
            fd_to_index_.erase(it);
        }

        size_t ConnectionPool::available() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return free_indices_.size();
        }

        std::vector<int> ConnectionPool::get_idle_connections(
                std::chrono::seconds timeout) {

            std::lock_guard<std::mutex> lock(mutex_);
            std::vector<int> idle_fds;
            auto now = std::chrono::steady_clock::now();

            for (const auto& pair : fd_to_index_) {
                int fd = pair.first;
                size_t idx = pair.second;

                const auto& entry = pool_[idx];
                if (entry.in_use && 
                        (now - entry.last_used) > timeout) {
                    idle_fds.push_back(fd);
                }
            }

            return idle_fds;
        }

    } // namespace server
} // namespace eventcore
