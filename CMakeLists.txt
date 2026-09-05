cmake_minimum_required(VERSION 3.20)

project(Heterogeneous_SoC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cache_tests
    src/cache.cpp
    tests/test_cache.cpp
)

target_include_directories(cache_tests PRIVATE
    include
)