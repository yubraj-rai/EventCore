cmake_minimum_required(VERSION 3.12)
project(EventCore VERSION 1.0.0 LANGUAGES CXX)

set(PROJECT_DESCRIPTION "High-performance C++14 HTTP Server Framework")
set(PROJECT_URL "https://github.com/yubraj-rai/EventCore")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Include modules
include(CompilerOptions)
include(FindDependencies)

# Build options
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(EVENTCORE_SOURCES
    src/core/logger.cpp
    src/net/socket.cpp
    src/net/address.cpp
    src/net/buffer.cpp
    src/net/poller.cpp
    src/http/request.cpp
    src/http/response.cpp
    src/http/parser.cpp
    src/http/router.cpp
    src/http/connection.cpp
    src/thread/thread_pool.cpp
    src/server/server.cpp
    src/server/worker.cpp
    src/server/connection_pool.cpp
)

# Static library
add_library(eventcore_static STATIC ${EVENTCORE_SOURCES})
target_link_libraries(eventcore_static PUBLIC Threads::Threads)
target_include_directories(eventcore_static PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
set_target_properties(eventcore_static PROPERTIES
    OUTPUT_NAME eventcore_static
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Shared library
if(BUILD_SHARED_LIBS)
    add_library(eventcore SHARED ${EVENTCORE_SOURCES})
    target_link_libraries(eventcore PUBLIC Threads::Threads)
    target_include_directories(eventcore PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    set_target_properties(eventcore PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endif()

# Executable
add_executable(eventcore_server src/main.cpp)
target_link_libraries(eventcore_server PRIVATE eventcore_static)
set_target_properties(eventcore_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Subdirectories
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Installation
include(cmake/InstallConfig.cmake)

# Package configuration
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/EventCoreConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EventCoreConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/EventCoreConfig.cmake"
    @ONLY
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/EventCoreConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/EventCoreConfigVersion.cmake"
    DESTINATION lib/cmake/EventCore
)

# pkg-config
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/eventcore.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/eventcore.pc"
    @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/eventcore.pc"
    DESTINATION lib/pkgconfig
)

# Configuration summary
message(STATUS "")
message(STATUS "EventCore Configuration Summary")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "  Build Shared Libraries: ${BUILD_SHARED_LIBS}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
message(STATUS "  Build Examples: ${BUILD_EXAMPLES}")
message(STATUS "  Build Benchmarks: ${BUILD_BENCHMARKS}")
message(STATUS "")

# Uninstall target
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
    )
    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    )
endif()
