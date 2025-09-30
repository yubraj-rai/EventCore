# EventCore - High-Performance C++ HTTP Server Framework

EventCore (pronounced "event core") is a high-performance C++ HTTP Server Framework designed to handle millions of concurrent connections with minimal latency. Built with architecture inspired by nginx, it provides both a standalone HTTP server and a reusable library for building custom network applications.

EventCore is free and open source software, distributed under the terms of the [MIT License](LICENSE).

> [!IMPORTANT]
> The goal of this README is to provide a basic, structured introduction to EventCore for developers. Please refer to the [full documentation](docs/) for detailed information on building, configuring, API reference, and more. These documentation pages also contain examples, performance guides, and complete class references.

## Table of contents [content pending]
- [How it works](#how-it-works)
  - [Architecture](#architecture)
  - [Components](#components)
  - [Configuration](#configuration)
- [Downloading and installing](#downloading-and-installing)
  - [Binary installation](#binary-installation)
  - [Building from source](#building-from-source)
  - [Package managers](#package-managers)
- [Getting started with EventCore](#getting-started-with-eventcore)
  - [Quick start example](#quick-start-example)
  - [Basic configuration](#basic-configuration)
  - [Running the server](#running-the-server)
- [Core features](#core-features)
  - [HTTP server](#http-server)
  - [Routing](#routing)
  - [Middleware](#middleware)
  - [Performance tuning](#performance-tuning)
- [Building from source](#building-from-source-1)
  - [Prerequisites](#prerequisites)
  - [Cloning the repository](#cloning-the-repository)
  - [Configuring the build](#configuring-the-build)
  - [Compiling](#compiling)
  - [Installation](#installation)
- [Asking questions and reporting issues](#asking-questions-and-reporting-issues)
- [Contributing code](#contributing-code)
- [Additional help and resources](#additional-help-and-resources)
- [Changelog](#changelog)
- [License](#license)

## How it works
EventCore is a C++ framework with source code available for all major POSIX-compliant operating systems. See Platform Support for a full list of compatible systems.

> [!IMPORTANT]
> While you can build EventCore from source on many systems, we recommend using the official build scripts for optimal performance and feature compatibility.

### Architecture
EventCore follows a multi-process, event-driven architecture:

- **Master Process**: Manages worker processes and configuration
- **Worker Processes**: Handle incoming connections (one per CPU core by default)  
- **Thread Pools**: Per-worker thread pools for CPU-intensive tasks
- **Connection Pool**: Pre-allocated connection objects for zero-allocation request handling

### Components
EventCore is comprised of modular C++ components, each providing specific functionality:

- **Core**: Logging, utilities, and base classes
- **Net**: Socket management, I/O multiplexing, buffers
- **HTTP**: Protocol implementation, parsing, routing
- **Thread**: Thread pools, synchronization primitives
- **Server**: High-level server framework and configuration

> [!TIP]
> You can use individual components independently or the complete server framework depending on your needs. Each component is designed as a standalone library.

### Configuration
EventCore is highly configurable through C++ API structures. Key configuration aspects include:

- Worker process count
- Thread pool sizes per worker
- Connection pool size
- Buffer sizes
- Timeout values
- TCP options (nodelay, reuseport, keepalive)

### Quick start example
```cpp
#include "eventcore/server/server.h"
#include <iostream>

int main() {
    eventcore::server::Config config;
    config.port = 8080;
    config.num_workers = 2;

    eventcore::server::Server server(config);

    // Simple route
    server.router().get("/", [](const eventcore::http::Request& req) {
        eventcore::http::Response resp;
        resp.set_status(200);
        resp.set_content_type("text/html");
        resp.set_body("<h1>Welcome to EventCore!</h1>");
        return resp;
    });

    // JSON API endpoint
    server.router().get("/api/health", [](const eventcore::http::Request& req) {
        return eventcore::http::Response::make_json(200, 
            R"({"status": "healthy", "server": "EventCore"})");
    });

    server.start();
    std::cout << "Server running on http://localhost:8080" << std::endl;
    server.wait();
    return 0;
}
