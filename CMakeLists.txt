cmake_minimum_required(VERSION 3.21)

# Disable modules support, this is broken with clang-tidy at the moment
if(POLICY CMP0155)
	cmake_policy(SET CMP0155 OLD)
endif()

project(challenge_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(plugin)
add_subdirectory(plugin_segfault)
add_subdirectory(app)

include(CTest)
if(BUILD_TESTING)
	add_subdirectory(tests)
endif()
