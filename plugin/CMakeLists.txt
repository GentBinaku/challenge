add_library(plugin SHARED src/plugin.cpp)

set_target_properties(
  plugin PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
                    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

target_include_directories(plugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)
    target_compile_definitions(plugin PRIVATE PLUGIN_BUILDING)
endif()

find_package(Boost REQUIRED COMPONENTS log log_setup)
target_link_libraries(plugin PRIVATE Boost::log Boost::log_setup)
