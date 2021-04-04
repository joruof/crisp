project(crisp)
cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(crisp_test test.cpp)

target_compile_options(crisp_test PUBLIC --std=c++17 -Wall -pedantic -Wextra)
