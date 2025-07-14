cmake_minimum_required(VERSION 3.10)
project(flexiframe CXX)

# option(EZCMD3_TEST "Build EZCMD3 test program" ON)

if(FLEXIFRAME_TESTS)
    add_executable(flexiframe_test test.cpp flexiframe.cpp)
    target_compile_definitions(flexiframe_test PRIVATE FLEXIFRAME_TESTS)
    target_include_directories(flexiframe_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()