cmake_minimum_required(VERSION 3.21)

# Disable modules support, this is broken with clang-tidy at the moment
if(POLICY CMP0155)
  cmake_policy(SET CMP0155 OLD)
endif()

project(challenge_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_SANITIZERS "Enable AddressSanitizer (GCC/Clang only)" OFF)
if(ENABLE_SANITIZERS)
  if(MSVC)
    message(FATAL_ERROR "ENABLE_SANITIZERS is not supported on MSVC")
  endif()
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address)
endif()

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
  # Use DT_RPATH instead of DT_RUNPATH so the search path propagates to
  # transitive dependencies (e.g. libboost_log -> libboost_filesystem).
  add_link_options(-Wl,--disable-new-dtags)
endif()

add_subdirectory(plugin)
add_subdirectory(plugin_segfault)
add_subdirectory(app)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()


if(CMAKE_EXPORT_COMPILE_COMMANDS)
    file(CREATE_LINK
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${CMAKE_SOURCE_DIR}/compile_commands.json"
        SYMBOLIC)
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
