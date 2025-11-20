# Examples for EventCore

# Example server
add_executable(example_server example_server.cpp)
target_link_libraries(example_server PRIVATE eventcore_static)
eventcore_setup_target(example_server)

# Simple API example
add_executable(simple_api simple_api.cpp)
target_link_libraries(simple_api PRIVATE eventcore_static)
eventcore_setup_target(simple_api)

# Static file server example
add_executable(static_file_server static_file_server.cpp)
target_link_libraries(static_file_server PRIVATE eventcore_static)
eventcore_setup_target(static_file_server)

# Logger example
add_executable(logger_example logger_example.cpp)
target_link_libraries(logger_example PRIVATE eventcore_static)
eventcore_setup_target(logger_example)

# Set output directories
set_target_properties(
    example_server 
    simple_api 
    static_file_server
    logger_example
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples
)

# Install examples
install(TARGETS 
    example_server 
    simple_api 
    static_file_server
    logger_example
    RUNTIME DESTINATION bin/examples
)
