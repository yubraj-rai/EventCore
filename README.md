# EventCore - High-Performance C++14 HTTP Server Framework

EventCore is a modern, high-performance HTTP server framework written in C++14.

## Features

- 🚀 High Performance: Event-driven architecture
- 🧵 Multi-threaded: Worker thread pools  
- 🔄 HTTP/1.1: Full protocol support
- 🛣️ Flexible Routing: Regex-based routing
- 📦 Easy to Use: Simple API

## Quick Start

```bash
git clone https://github.com/yourusername/eventcore.git
cd eventcore
mkdir build && cd build
    cmake ..
make -j$(nproc)
    ./eventcore_server
