#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace eventcore {
    namespace thread {

        /**
         * @brief A thread-safe blocking queue implementation
         * 
         * This queue blocks on pop operations when empty, making it suitable
         * for producer-consumer scenarios in thread pools.
         */
        template<typename T>
            class BlockingQueue {
                public:
                    /**
                     * @brief Push a value into the queue
                     * @param value The value to push
                     */
                    void push(T value) {
                        {
                            std::lock_guard<std::mutex> lock(mutex_);
                            queue_.push(std::move(value));
                        }
                        condition_.notify_one();
                    }

                    /**
                     * @brief Try to pop a value without blocking
                     * @param value Reference to store the popped value
                     * @return true if a value was popped, false if queue was empty
                     */
                    bool try_pop(T& value) {
                        std::lock_guard<std::mutex> lock(mutex_);
                        if (queue_.empty()) {
                            return false;
                        }
                        value = std::move(queue_.front());
                        queue_.pop();
                        return true;
                    }

                    /**
                     * @brief Pop a value from the queue, blocking if empty
                     * @return The popped value
                     */
                    T pop() {
                        std::unique_lock<std::mutex> lock(mutex_);
                        condition_.wait(lock, [this] { 
                                return !queue_.empty() || stopped_; 
                                });

                        if (stopped_ && queue_.empty()) {
                            throw std::runtime_error("Queue is stopped and empty");
                        }

                        T value = std::move(queue_.front());
                        queue_.pop();
                        return value;
                    }

                    /**
                     * @brief Try to pop a value with timeout
                     * @param value Reference to store the popped value
                     * @param timeout_ms Timeout in milliseconds
                     * @return true if a value was popped, false if timeout occurred
                     */
                    bool try_pop(T& value, int timeout_ms) {
                        std::unique_lock<std::mutex> lock(mutex_);

                        if (!condition_.wait_for(lock, std::chrono::milliseconds(timeout_ms), 
                                    [this] { return !queue_.empty() || stopped_; })) {
                            return false; // Timeout
                        }

                        if (stopped_ && queue_.empty()) {
                            return false;
                        }

                        value = std::move(queue_.front());
                        queue_.pop();
                        return true;
                    }

                    /**
                     * @brief Check if the queue is empty
                     * @return true if empty, false otherwise
                     */
                    bool empty() const {
                        std::lock_guard<std::mutex> lock(mutex_);
                        return queue_.empty();
                    }

                    /**
                     * @brief Get the number of elements in the queue
                     * @return Queue size
                     */
                    size_t size() const {
                        std::lock_guard<std::mutex> lock(mutex_);
                        return queue_.size();
                    }

                    /**
                     * @brief Stop the queue, waking up all waiting threads
                     */
                    void stop() {
                        {
                            std::lock_guard<std::mutex> lock(mutex_);
                            stopped_ = true;
                        }
                        condition_.notify_all();
                    }

                    /**
                     * @brief Restart the queue after being stopped
                     */
                    void restart() {
                        {
                            std::lock_guard<std::mutex> lock(mutex_);
                            stopped_ = false;
                        }
                    }

                    /**
                     * @brief Check if the queue is stopped
                     * @return true if stopped, false otherwise
                     */
                    bool is_stopped() const {
                        std::lock_guard<std::mutex> lock(mutex_);
                        return stopped_;
                    }

                    /**
                     * @brief Clear all elements from the queue
                     */
                    void clear() {
                        std::lock_guard<std::mutex> lock(mutex_);
                        while (!queue_.empty()) {
                            queue_.pop();
                        }
                    }

                private:
                    mutable std::mutex mutex_;
                    std::condition_variable condition_;
                    std::queue<T> queue_;
                    bool stopped_ = false;
            };

    } // namespace thread
} // namespace eventcore
