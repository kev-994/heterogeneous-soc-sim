cmake_minimum_required(VERSION 3.20)

project(Heterogeneous_SoC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(types_tests
    tests/test_types.cpp
)

target_include_directories(types_tests PRIVATE
    include
)

add_executable(config_tests
    tests/test_config.cpp
)

target_include_directories(config_tests PRIVATE
    include
)

add_executable(memory_tests
    src/memory.cpp
    tests/test_memory.cpp
)

target_include_directories(memory_tests PRIVATE
    include
)