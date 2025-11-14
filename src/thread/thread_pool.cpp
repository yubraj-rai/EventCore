#include "eventcore/thread/thread_pool.h"
#include "eventcore/core/logger.h"
#include <algorithm>
#include <sstream>

namespace eventcore {
    namespace thread {

        ThreadPool::ThreadPool(size_t num_threads) : num_threads_(num_threads) {
            threads_.reserve(num_threads_);
        }

        ThreadPool::~ThreadPool() { 
            stop(); 
        }

        void ThreadPool::start() {
            if (running_) return;
            running_ = true;
            for (size_t i = 0; i < num_threads_; ++i) {
                threads_.emplace_back(&ThreadPool::worker_thread, this);
            }

            std::stringstream ss;
            ss << "ThreadPool started with " << threads_.size() << " threads";
            LOG_INFO(ss.str());
        }

        void ThreadPool::stop() {
            if (!running_) return;
            running_ = false;

            // Stop the underlying queue to wake up all waiting threads.
            tasks_.stop();

            // Join all threads
            for (auto& thread : threads_) {
                if (thread.joinable()) thread.join();
            }
            threads_.clear();

            LOG_INFO("ThreadPool stopped");
        }

        void ThreadPool::submit(Task task) { 
            tasks_.push(std::move(task)); 
        }

        void ThreadPool::worker_thread() {
            while (true) {
                try {
                    Task task = tasks_.pop();
                    if (task) task();
                } catch (const std::exception& e) {
                    // Queue stopped or fatal error - exit thread
                    std::stringstream ss;
                    ss << "Worker exiting: " << e.what();
                    LOG_INFO(ss.str());
                    break;
                }
            }
        }

    } // namespace thread
} // namespace eventcore
