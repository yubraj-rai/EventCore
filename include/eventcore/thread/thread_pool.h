#pragma once
#include "blocking_queue.h"
#include "../core/noncopyable.h"
#include <vector>
#include <thread>
#include <functional>
#include <atomic>

namespace eventcore {
    namespace thread {

        class ThreadPool : public NonCopyable {
            public:
                using Task = std::function<void()>;
                explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
                ~ThreadPool();
                void start();
                void stop();
                void submit(Task task);
                size_t size() const { return threads_.size(); }
                size_t pending_tasks() const { return tasks_.size(); }

            private:
                void worker_thread();
                std::vector<std::thread> threads_;
                size_t num_threads_ = 0;
                BlockingQueue<Task> tasks_;
                std::atomic<bool> running_{false};
        };

    } // namespace thread
} // namespace eventcore
