#include <benchmark/benchmark.h>
#include "eventcore/server/server.h"
#include "eventcore/http/router.h"
#include "eventcore/net/socket.h"
#include <thread>
#include <atomic>
#include <vector>

class EventCoreFixture : public benchmark::Fixture {
    public:
        void SetUp(const benchmark::State& state) override {
            // Configure server
            config_.port = 0; // Let OS choose port
            config_.num_workers = state.range(0);
            config_.num_threads_per_worker = state.range(1);

            server_ = std::make_unique<eventcore::server::Server>(config_);

            // Setup simple routes for benchmarking
            server_->router().get("/hello", [](const eventcore::http::Request& req) {
                    eventcore::http::Response resp;
                    resp.set_status(200);
                    resp.set_content_type("text/plain");
                    resp.set_body("Hello, World!");
                    return resp;
                    });

            server_->router().get("/json", [](const eventcore::http::Request& req) {
                    eventcore::http::Response resp;
                    resp.set_status(200);
                    resp.set_content_type("application/json");
                    resp.set_body(R"({"message": "Hello", "status": "ok"})");
                    return resp;
                    });

            server_->router().post("/echo", [](const eventcore::http::Request& req) {
                    eventcore::http::Response resp;
                    resp.set_status(200);
                    resp.set_content_type("application/json");
                    resp.set_body(R"({"echo": ")" + req.body() + "\"}");
                    return resp;
                    });

            server_->start();

            // Get actual port (in real implementation, you'd need to expose this)
            port_ = 8080; // For demo purposes
        }

        void TearDown(const benchmark::State& state) override {
            server_->stop();
        }

        int port() const { return port_; }

    protected:
        eventcore::server::Config config_;
        std::unique_ptr<eventcore::server::Server> server_;
        int port_;
};

// Benchmark HTTP request processing throughput
BENCHMARK_DEFINE_F(EventCoreFixture, BM_RequestThroughput)(benchmark::State& state) {
    const std::string request = 
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";

    for (auto _ : state) {
        // In a real benchmark, you'd send actual HTTP requests
        // For now, we'll simulate the request processing
        eventcore::http::Request req;
        req.set_method(eventcore::http::Method::GET);
        req.set_path("/hello");
        req.set_version(eventcore::http::Version::HTTP_1_1);

        auto response = server_->router().route(req);
        benchmark::DoNotOptimize(response.to_string());
    }

    state.SetItemsProcessed(state.iterations());
}

// Benchmark different worker configurations
    BENCHMARK_REGISTER_F(EventCoreFixture, BM_RequestThroughput)
->Args({1, 1})    // 1 worker, 1 thread
->Args({2, 2})    // 2 workers, 2 threads each
->Args({4, 4})    // 4 workers, 4 threads each
->Args({8, 2})    // 8 workers, 2 threads each
    ->Unit(benchmark::kMillisecond);

    // Benchmark JSON response generation
    BENCHMARK_DEFINE_F(EventCoreFixture, BM_JsonResponse)(benchmark::State& state) {
        for (auto _ : state) {
            eventcore::http::Request req;
            req.set_method(eventcore::http::Method::GET);
            req.set_path("/json");

            auto response = server_->router().route(req);
            benchmark::DoNotOptimize(response.to_string());
        }

        state.SetItemsProcessed(state.iterations());
    }

    BENCHMARK_REGISTER_F(EventCoreFixture, BM_JsonResponse)
->Args({2, 2})
    ->Unit(benchmark::kMicrosecond);

    // Benchmark POST request with body
    BENCHMARK_DEFINE_F(EventCoreFixture, BM_PostEcho)(benchmark::State& state) {
        for (auto _ : state) {
            eventcore::http::Request req;
            req.set_method(eventcore::http::Method::POST);
            req.set_path("/echo");
            req.set_body(R"({"test": "data", "value": 123})");

            auto response = server_->router().route(req);
            benchmark::DoNotOptimize(response.to_string());
        }

        state.SetItemsProcessed(state.iterations());
    }

    BENCHMARK_REGISTER_F(EventCoreFixture, BM_PostEcho)
->Args({2, 2})
    ->Unit(benchmark::kMicrosecond);

    // Router performance benchmark
    static void BM_RouterMatching(benchmark::State& state) {
        eventcore::http::Router router;

        // Add multiple routes
        for (int i = 0; i < state.range(0); ++i) {
            router.get("/api/v" + std::to_string(i) + "/users/" + std::to_string(i), 
                    [](const eventcore::http::Request& req) {
                    return eventcore::http::Response::make_json(200, R"({"result": "ok"})");
                    });
        }

        eventcore::http::Request req;
        req.set_method(eventcore::http::Method::GET);
        req.set_path("/api/v1/users/1");

        for (auto _ : state) {
            auto response = router.route(req);
            benchmark::DoNotOptimize(response.to_string());
        }

        state.SetItemsProcessed(state.iterations());
    }

    BENCHMARK(BM_RouterMatching)
    ->Arg(10)    // 10 routes
    ->Arg(100)   // 100 routes
->Arg(1000)  // 1000 routes
    ->Unit(benchmark::kMicrosecond);

    // Buffer performance benchmarks
    static void BM_BufferAppend(benchmark::State& state) {
        const std::string data(1024, 'x'); // 1KB of data

        for (auto _ : state) {
            eventcore::net::Buffer buffer;
            for (int i = 0; i < state.range(0); ++i) {
                buffer.append(data);
            }
            benchmark::DoNotOptimize(buffer.retrieve_all_as_string());
        }

        state.SetBytesProcessed(state.iterations() * state.range(0) * data.size());
    }

    BENCHMARK(BM_BufferAppend)
    ->Arg(10)    // 10 appends
    ->Arg(100)   // 100 appends
->Arg(1000)  // 1000 appends
    ->Unit(benchmark::kMicrosecond);

    // Thread pool performance
    static void BM_ThreadPoolThroughput(benchmark::State& state) {
        eventcore::thread::ThreadPool pool(state.range(0));
        pool.start();

        std::atomic<int> counter{0};

        for (auto _ : state) {
            const int tasks = state.range(1);
            counter.store(0);

            for (int i = 0; i < tasks; ++i) {
                pool.submit([&counter]() {
                        // Simulate some work
                        volatile int x = 0;
                        for (int j = 0; j < 1000; ++j) {
                        x += j;
                        }
                        counter.fetch_add(1, std::memory_order_relaxed);
                        });
            }

            // Wait for all tasks to complete
            while (counter.load(std::memory_order_relaxed) < tasks) {
                std::this_thread::yield();
            }
        }

        pool.stop();
        state.SetItemsProcessed(state.iterations() * state.range(1));
    }

    BENCHMARK(BM_ThreadPoolThroughput)
->Args({1, 100})    // 1 thread, 100 tasks
->Args({2, 100})    // 2 threads, 100 tasks
->Args({4, 100})    // 4 threads, 100 tasks
->Args({8, 100})    // 8 threads, 100 tasks
->Args({4, 1000})   // 4 threads, 1000 tasks
    ->Unit(benchmark::kMillisecond);

    // HTTP parser performance
    static void BM_HttpParser(benchmark::State& state) {
        const std::string http_request = 
            "POST /api/data HTTP/1.1\r\n"
            "Host: localhost:8080\r\n"
            "User-Agent: Benchmark/1.0\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: 27\r\n"
            "Connection: keep-alive\r\n"
            "\r\n"
            R"({"message": "Hello, World!"})";

        eventcore::net::Buffer buffer;
        buffer.append(http_request);

        for (auto _ : state) {
            eventcore::http::Parser parser;
            eventcore::http::Request request;

            bool result = parser.parse_request(&buffer, &request);
            benchmark::DoNotOptimize(result);
            benchmark::DoNotOptimize(request.body());

            // Reset for next iteration
            parser.reset();
            buffer.retrieve_all();
            buffer.append(http_request);
        }

        state.SetItemsProcessed(state.iterations());
    }

BENCHMARK(BM_HttpParser)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
