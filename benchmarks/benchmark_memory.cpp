#include <benchmark/benchmark.h>
#include "eventcore/server/server.h"
#include "eventcore/http/router.h"
#include "eventcore/net/buffer.h"
#include "eventcore/thread/thread_pool.h"
#include <memory>
#include <vector>

// Memory usage benchmarks for EventCore components

static void BM_BufferMemoryUsage(benchmark::State& state) {
    const size_t buffer_size = state.range(0);
    const std::string data(buffer_size, 'x');

    size_t total_memory = 0;
    size_t allocation_count = 0;

    for (auto _ : state) {
        eventcore::net::Buffer buffer;

        // Measure memory after appending data
        buffer.append(data);

        // Estimate memory usage (this is approximate)
        size_t estimated_memory = buffer_size + eventcore::net::Buffer::kPrependSize;
        total_memory += estimated_memory;
        allocation_count++;

        benchmark::DoNotOptimize(buffer.retrieve_all_as_string());
    }

    state.counters["AvgBufferSize"] = benchmark::Counter(
            total_memory / allocation_count, benchmark::Counter::kAvgThreads);
    state.counters["MemoryBandwidth"] = benchmark::Counter(
            total_memory, benchmark::Counter::kIsRate);
}

    BENCHMARK(BM_BufferMemoryUsage)
    ->Arg(64)      // 64 bytes
    ->Arg(1024)    // 1KB
    ->Arg(4096)    // 4KB
    ->Arg(16384)   // 16KB
->Arg(65536)   // 64KB
    ->Unit(benchmark::kMicrosecond);

    static void BM_ConnectionMemoryOverhead(benchmark::State& state) {
        const int num_connections = state.range(0);

        for (auto _ : state) {
            std::vector<std::shared_ptr<eventcore::http::Connection>> connections;
            connections.reserve(num_connections);

            // Create mock connections (without actual sockets)
            for (int i = 0; i < num_connections; ++i) {
                // We can't easily create real connections without sockets,
                // so we'll measure the overhead of connection objects
                auto socket_result = eventcore::net::Socket::create_tcp();
                if (socket_result.is_ok()) {
                    auto conn = std::make_shared<eventcore::http::Connection>(
                            std::move(socket_result.value()),
                            [](const eventcore::http::Request&) {
                            return eventcore::http::Response::make_404();
                            }
                            );
                    connections.push_back(conn);
                }
            }

            benchmark::DoNotOptimize(connections.size());

            // Memory measurement would typically be done with external tools
            // For this benchmark, we just track the count
            state.counters["Connections"] = benchmark::Counter(
                    connections.size(), benchmark::Counter::kDefaults);
        }
    }

    BENCHMARK(BM_ConnectionMemoryOverhead)
    ->Arg(10)      // 10 connections
    ->Arg(100)     // 100 connections
    ->Arg(1000)    // 1000 connections
->Arg(10000)   // 10000 connections
    ->Unit(benchmark::kMillisecond);

    static void BM_ThreadPoolMemoryScaling(benchmark::State& state) {
        const int num_threads = state.range(0);
        const int tasks_per_thread = state.range(1);

        for (auto _ : state) {
            eventcore::thread::ThreadPool pool(num_threads);
            pool.start();

            std::atomic<int> completed_tasks{0};
            const int total_tasks = num_threads * tasks_per_thread;

            // Submit tasks to measure memory under load
            for (int i = 0; i < total_tasks; ++i) {
                pool.submit([&completed_tasks, size = state.range(2)]() {
                        // Allocate some memory to simulate real workload
                        std::vector<char> buffer(size);
                        for (size_t j = 0; j < buffer.size(); ++j) {
                        buffer[j] = static_cast<char>(j % 256);
                        }
                        completed_tasks.fetch_add(1, std::memory_order_relaxed);
                        benchmark::DoNotOptimize(buffer.data());
                        });
            }

            // Wait for completion
            while (completed_tasks.load(std::memory_order_relaxed) < total_tasks) {
                std::this_thread::yield();
            }

            pool.stop();

            state.counters["Threads"] = benchmark::Counter(num_threads, benchmark::Counter::kDefaults);
            state.counters["TasksPerThread"] = benchmark::Counter(tasks_per_thread, benchmark::Counter::kDefaults);
        }
    }

    BENCHMARK(BM_ThreadPoolMemoryScaling)
->Args({1, 10, 1024})     // 1 thread, 10 tasks, 1KB memory per task
->Args({2, 10, 1024})     // 2 threads, 10 tasks, 1KB memory per task
->Args({4, 10, 1024})     // 4 threads, 10 tasks, 1KB memory per task
->Args({8, 10, 1024})     // 8 threads, 10 tasks, 1KB memory per task
->Args({4, 100, 1024})    // 4 threads, 100 tasks, 1KB memory per task
->Args({4, 10, 4096})     // 4 threads, 10 tasks, 4KB memory per task
    ->Unit(benchmark::kMillisecond);

    static void BM_RouterMemoryUsage(benchmark::State& state) {
        const int num_routes = state.range(0);

        for (auto _ : state) {
            eventcore::http::Router router;

            // Add routes with different patterns
            for (int i = 0; i < num_routes; ++i) {
                router.get("/api/v" + std::to_string(i) + "/resource/" + std::to_string(i),
                        [i](const eventcore::http::Request& req) {
                        eventcore::http::Response resp;
                        resp.set_status(200);
                        resp.set_body("Response for route " + std::to_string(i));
                        return resp;
                        });

                router.post("/api/v" + std::to_string(i) + "/create",
                        [i](const eventcore::http::Request& req) {
                        eventcore::http::Response resp;
                        resp.set_status(201);
                        resp.set_body("Created resource " + std::to_string(i));
                        return resp;
                        });
            }

            // Measure memory by performing some route lookups
            eventcore::http::Request req;
            req.set_method(eventcore::http::Method::GET);
            req.set_path("/api/v1/resource/1");

            auto response = router.route(req);
            benchmark::DoNotOptimize(response.to_string());

            state.counters["Routes"] = benchmark::Counter(num_routes * 2, benchmark::Counter::kDefaults); // GET + POST
        }
    }

    BENCHMARK(BM_RouterMemoryUsage)
    ->Arg(10)      // 20 total routes (10 GET + 10 POST)
    ->Arg(50)      // 100 total routes
    ->Arg(100)     // 200 total routes
->Arg(500)     // 1000 total routes
    ->Unit(benchmark::kMillisecond);

    static void BM_ServerStartupMemory(benchmark::State& state) {
        const int num_workers = state.range(0);

        for (auto _ : state) {
            eventcore::server::Config config;
            config.num_workers = num_workers;
            config.num_threads_per_worker = state.range(1);

            auto server = std::make_unique<eventcore::server::Server>(config);

            // Add some routes
            server->router().get("/test", [](const eventcore::http::Request& req) {
                    return eventcore::http::Response::make_json(200, R"({"status": "ok"})");
                    });

            server->start();

            // Give server time to initialize
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            benchmark::DoNotOptimize(server);

            server->stop();

            state.counters["Workers"] = benchmark::Counter(num_workers, benchmark::Counter::kDefaults);
            state.counters["ThreadsPerWorker"] = benchmark::Counter(state.range(1), benchmark::Counter::kDefaults);
        }
    }

    BENCHMARK(BM_ServerStartupMemory)
->Args({1, 1})    // 1 worker, 1 thread
->Args({2, 2})    // 2 workers, 2 threads
->Args({4, 4})    // 4 workers, 4 threads
->Args({8, 2})    // 8 workers, 2 threads
    ->Unit(benchmark::kMillisecond);

    // Memory fragmentation test
    static void BM_MemoryFragmentation(benchmark::State& state) {
        const int iterations = state.range(0);
        const int buffer_size = state.range(1);

        for (auto _ : state) {
            std::vector<std::unique_ptr<eventcore::net::Buffer>> buffers;

            // Create and destroy buffers to simulate memory fragmentation
            for (int i = 0; i < iterations; ++i) {
                auto buffer = std::make_unique<eventcore::net::Buffer>();

                // Fill buffer with data
                std::string data(buffer_size, static_cast<char>('A' + (i % 26)));
                buffer->append(data);

                // Use the buffer
                auto content = buffer->retrieve_all_as_string();
                benchmark::DoNotOptimize(content);

                buffers.push_back(std::move(buffer));

                // Periodically clear some buffers to create fragmentation
                if (i % 10 == 0 && !buffers.empty()) {
                    buffers.pop_back();
                }
            }

            state.counters["BuffersCreated"] = benchmark::Counter(iterations, benchmark::Counter::kDefaults);
            state.counters["FinalBufferCount"] = benchmark::Counter(buffers.size(), benchmark::Counter::kDefaults);
        }
    }

    BENCHMARK(BM_MemoryFragmentation)
->Args({100, 1024})    // 100 iterations, 1KB buffers
->Args({1000, 1024})   // 1000 iterations, 1KB buffers
->Args({100, 4096})    // 100 iterations, 4KB buffers
->Args({1000, 4096})   // 1000 iterations, 4KB buffers
    ->Unit(benchmark::kMillisecond);

    // Response object memory usage
    static void BM_ResponseMemory(benchmark::State& state) {
        const int body_size = state.range(0);
        const int header_count = state.range(1);

        for (auto _ : state) {
            eventcore::http::Response resp;
            resp.set_status(200, "OK");

            // Add headers
            for (int i = 0; i < header_count; ++i) {
                resp.set_header("X-Custom-Header-" + std::to_string(i), 
                        "Value-" + std::to_string(i));
            }

            // Set body
            std::string body(body_size, 'x');
            resp.set_body(body);

            // Convert to string (simulates network send)
            auto response_str = resp.to_string();
            benchmark::DoNotOptimize(response_str);

            state.counters["BodySize"] = benchmark::Counter(body_size, benchmark::Counter::kDefaults);
            state.counters["HeaderCount"] = benchmark::Counter(header_count, benchmark::Counter::kDefaults);
            state.counters["ResponseSize"] = benchmark::Counter(response_str.size(), benchmark::Counter::kDefaults);
        }
    }

    BENCHMARK(BM_ResponseMemory)
->Args({1024, 5})     // 1KB body, 5 headers
->Args({10240, 5})    // 10KB body, 5 headers
->Args({1024, 20})    // 1KB body, 20 headers
->Args({10240, 20})   // 10KB body, 20 headers
    ->Unit(benchmark::kMicrosecond);

    BENCHMARK_MAIN();
