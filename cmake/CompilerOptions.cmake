# Compiler warning options
function(set_compiler_warnings target_name)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wcast-align
            -Wunused
            -Woverloaded-virtual
            -Wconversion
            -Wsign-conversion
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2
        )
        
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            target_compile_options(${target_name} PRIVATE
                -Wmisleading-indentation
                -Wduplicated-cond
                -Wduplicated-branches
                -Wlogical-op
                -Wuseless-cast
            )
        endif()
        
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${target_name} PRIVATE
                -Wthread-safety
                -Wloop-analysis
            )
        endif()
    elseif(MSVC)
        target_compile_options(${target_name} PRIVATE
            /W4
            /permissive-
            /wd4100 # unreferenced formal parameter
            /wd4127 # conditional expression is constant
            /wd4324 # structure was padded due to alignment specifier
        )
    endif()
endfunction()

# Platform-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG -D_DEBUG")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_ASSERTIONS")
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
        # Link Time Optimization
        if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.0)
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
        endif()
    endif()
    
    # Position Independent Code for shared libraries
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    
    # Visibility settings
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
endif()

# Set compiler warnings for all targets
function(eventcore_setup_target target)
    set_compiler_warnings(${target})
    
    # Set C++ standard properties
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    
    # Add include directories
    target_include_directories(${target} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
endfunction()
