# Tests
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    
    # Test executables
    add_executable(test_http test_http.cpp)
    target_link_libraries(test_http PRIVATE eventcore_static GTest::gtest GTest::gtest_main)
    set_target_properties(test_http PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
    )
    
    add_executable(test_net test_net.cpp)
    target_link_libraries(test_net PRIVATE eventcore_static GTest::gtest GTest::gtest_main)
    set_target_properties(test_net PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
    )
    
    add_executable(test_thread test_thread.cpp)
    target_link_libraries(test_thread PRIVATE eventcore_static GTest::gtest GTest::gtest_main)
    set_target_properties(test_thread PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
    )
    
    add_executable(test_server test_server.cpp)
    target_link_libraries(test_server PRIVATE eventcore_static GTest::gtest GTest::gtest_main)
    set_target_properties(test_server PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
    )
    
    # Add tests
    add_test(NAME test_http COMMAND test_http)
    add_test(NAME test_net COMMAND test_net)
    add_test(NAME test_thread COMMAND test_thread)
    add_test(NAME test_server COMMAND test_server)
    
    # Install tests (optional)
    if(INSTALL_TESTS)
        install(TARGETS test_http test_net test_thread test_server
            RUNTIME DESTINATION bin/tests
        )
    endif()
endif()
