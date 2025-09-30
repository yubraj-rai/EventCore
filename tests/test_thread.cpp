#include <gtest/gtest.h>
#include "eventcore/thread/blocking_queue.h"
#include "eventcore/thread/thread_pool.h"
#include <thread>
#include <atomic>
#include <chrono>

using namespace eventcore::thread;

TEST(BlockingQueueTest, BasicOperations) {
    BlockingQueue<int> queue;

    // Test push and pop
    queue.push(42);
    int value = 0;
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 42);

    // Test empty
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

TEST(BlockingQueueTest, MultipleElements) {
    BlockingQueue<int> queue;

    for (int i = 0; i < 10; ++i) {
        queue.push(i);
    }

    EXPECT_EQ(queue.size(), 10);

    for (int i = 0; i < 10; ++i) {
        int value = queue.pop();
        EXPECT_EQ(value, i);
    }

    EXPECT_TRUE(queue.empty());
}

TEST(BlockingQueueTest, BlockingPop) {
    BlockingQueue<int> queue;
    std::atomic<bool> item_popped{false};
    int popped_value = 0;

    // Start consumer thread
    std::thread consumer([&]() {
            popped_value = queue.pop();
            item_popped = true;
            });

    // Give consumer time to block
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(item_popped);

    // Push item and verify consumer gets it
    queue.push(100);
    consumer.join();

    EXPECT_TRUE(item_popped);
    EXPECT_EQ(popped_value, 100);
}

TEST(BlockingQueueTest, TimeoutPop) {
    BlockingQueue<int> queue;
    int value = 0;

    // Should timeout since queue is empty
    EXPECT_FALSE(queue.try_pop(value, 100)); // 100ms timeout
    EXPECT_EQ(value, 0); // Value shouldn't be modified
}

TEST(BlockingQueueTest, StopBehavior) {
    BlockingQueue<int> queue;

    // Push some items
    queue.push(1);
    queue.push(2);

    // Stop the queue
    queue.stop();

    // Should still be able to pop existing items
    int value = 0;
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 1);

    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 2);

    // Now queue should be empty and stopped
    EXPECT_TRUE(queue.empty());
    EXPECT_TRUE(queue.is_stopped());

    // Try pop on stopped empty queue should return false
    EXPECT_FALSE(queue.try_pop(value));
}

TEST(ThreadPoolTest, BasicFunctionality) {
    ThreadPool pool(2);
    pool.start();

    std::atomic<int> counter{0};

    // Submit tasks
    for (int i = 0; i < 10; ++i) {
        pool.submit([&counter]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                counter.fetch_add(1);
                });
    }

    // Wait for tasks to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(counter.load(), 10);
    pool.stop();
}

TEST(ThreadPoolTest, StopWithPendingTasks) {
    ThreadPool pool(1);
    pool.start();

    std::atomic<int> counter{0};

    // Submit a task that will block
    pool.submit([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            counter.fetch_add(1);
            });

    // Submit more tasks that might not complete
    for (int i = 0; i < 5; ++i) {
        pool.submit([&counter]() {
                counter.fetch_add(1);
                });
    }

    // Stop immediately
    pool.stop();

    // Only the first task should complete (maybe)
    // The behavior depends on timing, but at least one should complete
    EXPECT_GE(counter.load(), 1);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
