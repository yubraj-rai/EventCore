## Quick orientation — EventCore (what to know up front)

- High level: EventCore is a high-performance, POSIX-focused C++14 HTTP framework using a master acceptor + multiple worker processes. Each worker runs a single-threaded event loop (epoll on Linux), a per-worker thread pool for CPU-bound tasks, and a connection pool for zero-allocation reuse. See `docs/architecture.txt` for the full diagram and flow.

- Key components and example files:
  - core utilities & logging: `src/core/logger.cpp`, headers under `include/eventcore/core/`
  - networking primitives: `src/net/*` (notably `src/net/socket.cpp`) and `include/eventcore/net/`
  - HTTP layer: `src/http/*`, request/response/parsing and router
  - server orchestration: `src/server/*` and the public API in `include/eventcore/server/`
  - runnable server: `src/main.cpp` demonstrates tuning (setrlimit), signal handling, and basic router usage.

## Build & test workflow (exact commands)

- Primary helper script: `./build.sh` — use it. Examples:

  - Quick build (Release):
    ./build.sh --clean --build-type Release

  - Build, run tests and install to the default build prefix:
    ./build.sh --clean --test --install

  - Common cmake flow (manual):
    mkdir -p build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
    make -j$(nproc)

- Tests: CTest + GTest. After building in `build/` run:

    cd build && ctest --output-on-failure

  Test binaries are emitted to `build/bin/tests/` (see `tests/CMakeLists.txt`). The test suite uses `eventcore_static`.

## Project-specific conventions and patterns (do this in code changes)

- C++ standard: codebase uses C++14 (see `CMakeLists.txt`). Keep compatibility. Prefer small, focused changes.

- Error handling: hot paths avoid exceptions — the repo uses a Result<T> style (check return values). Follow this pattern in networking/IO code (see `src/net/socket.cpp`).

- Zero-allocation / pooling patterns:
  - Connection objects are pooled and reused. Avoid allocating per-request objects in hot paths.
  - Buffer reuse and prefer stack or pre-allocated buffers where appropriate.

- Concurrency model:
  - Per-worker isolation: do not introduce shared mutable global state across worker processes.
  - Event loop is single-threaded; offload CPU work to thread pools (see `src/thread/` implementations).
  - Use atomics and lock-free queues for cross-thread handoff when appropriate.

- Socket options & epoll usage:
  - Edge-triggered epoll (EPOLLET) and non-blocking sockets are the norm — follow patterns in `src/net/poller.cpp` and `src/net/socket.cpp`.
  - Use `Socket::set_nonblocking`, `set_nodelay`, `set_reuseport` helpers.

## Adding code, tests, or examples

- Where to add code:
  - Headers: `include/eventcore/<component>/...`
  - Sources: `src/<component>/...`
  - Add new source files to `CMakeLists.txt` list `EVENTCORE_SOURCES` at repository root so they are included in the library targets.

- Tests: add a new executable in `tests/CMakeLists.txt`. Use GTest and link against `eventcore_static`. Tests are registered with `add_test()` and placed in `build/bin/tests/`.

## Integration points / external deps

- Build-time: CMake, Threads, and Google Test (GTest) for unit tests. See `CMakeLists.txt` and `cmake/FindDependencies.cmake`.

- Runtime: POSIX sockets (epoll on Linux). Platform differences are handled in polling layer — prefer the abstractions in `src/net/poller.cpp`.

## Fast examples to look at

- `src/main.cpp` — canonical runnable server: shows `tune_system_limits()`, signal handlers, router usage and `server.start()` / `server.stop()` lifecycle.
- `docs/architecture.txt` — authoritative architecture diagram and performance/optimization notes.
- `src/net/socket.cpp` — canonical socket option helpers and Result<T> usage.

## Small operational notes for agents

- Preserve the no-exceptions-in-hot-paths pattern.
- When changing public APIs, update `include/` headers and `CMakeLists.txt` accordingly.
- Keep changes small and add/modify tests under `tests/` to guard behavior. Use `./build.sh --test` for CI-style verification.

If any part above is unclear or you want the file tailored for a different agent style (more examples, stricter PR rules, or explicit unit-test templates), tell me which sections to expand and I will iterate.
