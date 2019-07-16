cmake_minimum_required(VERSION 3.14)
project(figurer)

add_definitions(-std=c++11)
set(CMAKE_CXX_STANDARD 11)
set(CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS, "${CXX_FLAGS}")

configure_file(CMakeLists-flann.txt flann-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/flann-download )
if(result)
    message(FATAL_ERROR "CMake step for flann failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/flann-download )
if(result)
    message(FATAL_ERROR "Build step for flann failed: ${result}")
endif()
add_subdirectory(${CMAKE_BINARY_DIR}/flann-src ${CMAKE_BINARY_DIR}/flann-build)
include_directories(${CMAKE_BINARY_DIR}/flann-src/src/cpp/)
include_directories(/usr/local/Cellar/lz4/1.9.1/include)
link_directories(/usr/local/Cellar/lz4/1.9.1/lib)

set(boostzip ${CMAKE_BINARY_DIR}/boost_1_70_0.zip)
set(boostdir ${CMAKE_BINARY_DIR}/boost_1_70_0/)
set(boostheader ${boostdir}/boost/geometry/index/rtree.hpp)
if(NOT EXISTS ${boostheader})
    file(DOWNLOAD https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.zip ${boostzip})
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf ${boostzip}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
include_directories(${boostdir})

include_directories(${PROJECT_SOURCE_DIR}/src)
set(sources
        src/figurer.cpp src/figurer.hpp
        src/figurer_distribution.cpp src/figurer_distribution.hpp
        src/figurer_spatial_index.cpp src/figurer_spatial_index.hpp
        src/figurer_robot2d_example.cpp src/figurer_robot2d_example.cpp)
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

set(test_sources test/figurer_distribution_test.cpp test/figurer_robot2d_test.cpp test/spatial_index_test.cpp)
add_executable(test_figurer ${test_sources} ${sources})
target_link_libraries(test_figurer gtest_main)
