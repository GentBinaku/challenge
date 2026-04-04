cmake_minimum_required(VERSION 3.21)

# Disable modules support, this is broken with clang-tidy at the moment
if(POLICY CMP0155)
  cmake_policy(SET CMP0155 OLD)
endif()

project(challenge_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC multi-config generators append the config name (e.g. Debug/Release) to
# RUNTIME_OUTPUT_DIRECTORY unless the per-config variants are set explicitly.
# Keep all outputs flat so the Conan-copied DLLs and the built DLLs share one
# directory and the loader can find everything without extra PATH gymnastics.
foreach(_cfg IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
  string(TOUPPER "${_cfg}" _CFG)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_CFG} "${CMAKE_BINARY_DIR}/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_CFG} "${CMAKE_BINARY_DIR}/lib")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_CFG} "${CMAKE_BINARY_DIR}/lib")
endforeach()

# RPATH settings: make the installed tree relocatable on Linux.
# Plugins land in <prefix>/lib/ and the executable in <prefix>/bin/.
# $ORIGIN lets each binary find its siblings; $ORIGIN/../lib lets the
# executable (in bin/) find shared libs in lib/.
if(NOT WIN32)
  set(CMAKE_INSTALL_RPATH "$ORIGIN;$ORIGIN/../lib")
  set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
endif()

add_subdirectory(plugin)
add_subdirectory(plugin_segfault)
add_subdirectory(app)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

# Install third-party shared libraries copied by conanfile.py
if(WIN32)
  install(
    DIRECTORY "${CMAKE_BINARY_DIR}/bin/"
    DESTINATION bin
    FILES_MATCHING
    PATTERN "*.dll")
else()
  install(
    DIRECTORY "${CMAKE_BINARY_DIR}/lib/"
    DESTINATION lib
    FILES_MATCHING
    PATTERN "*.so*")
endif()
