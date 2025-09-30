# Build with benchmarks
# preq: sudo apt-get install libbenchmark-dev

cd EventCore
mkdir build && cd build
cmake -DBUILD_BENCHMARKS=ON ..
make -j$(nproc)

# Run throughput benchmarks
./benchmarks/benchmark_throughput --benchmark_time_unit=ms

# Run memory benchmarks  
./benchmarks/benchmark_memory --benchmark_time_unit=ms

# Run specific benchmark with filters
./benchmarks/benchmark_throughput --benchmark_filter="BM_RequestThroughput"

# Run with detailed output
./benchmarks/benchmark_throughput --benchmark_format=json
