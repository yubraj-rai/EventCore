# Benchmarks for EventCore
if(BUILD_BENCHMARKS)
    find_package(benchmark QUIET)
    
    if(benchmark_FOUND)
        # Throughput benchmark
        add_executable(benchmark_throughput benchmark_throughput.cpp)
        target_link_libraries(benchmark_throughput PRIVATE eventcore_static benchmark::benchmark)
        set_target_properties(benchmark_throughput PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/benchmarks
        )
        
        # Memory benchmark
        add_executable(benchmark_memory benchmark_memory.cpp)
        target_link_libraries(benchmark_memory PRIVATE eventcore_static benchmark::benchmark)
        set_target_properties(benchmark_memory PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/benchmarks
        )
        
        # Install benchmarks
        install(TARGETS benchmark_throughput benchmark_memory
            RUNTIME DESTINATION bin/benchmarks
        )
        
        message(STATUS "Google Benchmark found - building benchmarks")
    else()
        message(WARNING "Google Benchmark not found - skipping benchmarks")
    endif()
endif()
