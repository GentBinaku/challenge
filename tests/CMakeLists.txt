find_package(GTest REQUIRED)

add_executable(plugin_segfault_tests test_plugin_segfault.cpp)

set_target_properties(
  plugin_segfault_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                   "${CMAKE_BINARY_DIR}/bin")
target_include_directories(
  plugin_segfault_tests PRIVATE ${PROJECT_SOURCE_DIR}/plugin_segfault/include)

target_link_libraries(plugin_segfault_tests PRIVATE GTest::gtest)

if(UNIX AND NOT APPLE)
    target_link_libraries(plugin_segfault_tests PRIVATE dl)
endif()

add_test(
  NAME plugin_segfault_tests
  COMMAND plugin_segfault_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set_tests_properties(
  plugin_segfault_tests
  PROPERTIES ENVIRONMENT "PLUGIN_SEGFAULT_PATH=$<TARGET_FILE:plugin_segfault>;LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib")
