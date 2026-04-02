find_package(GTest REQUIRED)

add_executable(plugin_tests
    test_plugin.cpp
)

set_target_properties(plugin_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(plugin_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/plugin/include
)

target_link_libraries(plugin_tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
)

if(UNIX AND NOT APPLE)
    target_link_libraries(plugin_tests PRIVATE dl)
endif()

add_test(NAME plugin_tests COMMAND plugin_tests)
