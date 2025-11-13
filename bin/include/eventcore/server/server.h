#pragma once
#include "../core/noncopyable.h"
#include "../net/socket.h"
#include "../http/router.h"
#include "connection_pool.h"
#include "config.h"
#include "worker.h"
#include <memory>
#include <vector>
#include <atomic>

namespace eventcore {
    namespace server {

        class Server : public NonCopyable {
            public:
                explicit Server(const Config& config = Config());
                ~Server();
                void start();
                void stop();
                void wait();
                http::Router& router() { return router_; }
                const Config& config() const { return config_; }
                bool is_running() const { return running_; }

            private:
                void accept_loop();
                void handle_new_connection(net::Socket client_socket);
                Worker* next_worker();

                Config config_;
                ConnectionPool pool_;
                http::Router router_;
                net::Socket listen_socket_;
                std::vector<std::unique_ptr<Worker>> workers_;
                std::thread accept_thread_;
                std::atomic<bool> running_{false};
                std::atomic<size_t> next_worker_idx_{0};
        };

    } // namespace server
} // namespace eventcore
