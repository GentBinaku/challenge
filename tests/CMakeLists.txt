find_package(GTest REQUIRED)

file(GLOB TEST_SOURCES "*.cpp")

add_executable(plugin_tests ${TEST_SOURCES})

set_target_properties(plugin_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                              "${CMAKE_BINARY_DIR}/bin")

target_include_directories(plugin_tests
                           PRIVATE ${PROJECT_SOURCE_DIR}/plugin/include)

target_link_libraries(plugin_tests PRIVATE GTest::gtest GTest::gtest_main)

if(UNIX AND NOT APPLE)
    target_link_libraries(plugin_tests PRIVATE dl)
endif()

add_test(NAME plugin_tests COMMAND plugin_tests)

set_tests_properties(plugin_tests
                     PROPERTIES ENVIRONMENT
                     "PLUGIN_PATH=$<TARGET_FILE:plugin>;LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib")
