cmake_minimum_required(VERSION 3.14)
project(figurer)

add_definitions(-std=c++14)
set(CMAKE_CXX_STANDARD 14)
set(CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS, "${CXX_FLAGS}")

include_directories(${PROJECT_SOURCE_DIR}/src)
set(sources src/figurer.cpp src/figurer.hpp src/figurer_distribution.cpp src/figurer_distribution.hpp)
add_library(figurer ${sources})

configure_file(CMakeLists-googletest.txt googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src ${CMAKE_BINARY_DIR}/googletest-build)

set(test_sources test/figurer_test.cpp)
add_executable(test_figurer ${test_sources} ${sources})
target_link_libraries(test_figurer gtest_main)
