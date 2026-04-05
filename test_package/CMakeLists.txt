cmake_minimum_required(VERSION 3.21)
project(test_package LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(challenge_project REQUIRED)

add_executable(test_package src/main.cpp)
target_link_libraries(test_package PRIVATE challenge_project::plugin challenge_project::plugin_segfault)
