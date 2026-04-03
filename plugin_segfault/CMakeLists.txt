add_library(plugin_segfault SHARED src/plugin_segfault.cpp)

set_target_properties(
  plugin_segfault
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
             LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
             ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

target_include_directories(plugin_segfault
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)
  target_compile_definitions(
    plugin_segfault PRIVATE PLUGIN_BUILDING BOOST_STACKTRACE_USE_WINDBG)
  target_link_libraries(plugin_segfault PRIVATE dbghelp ole32)

  find_package(Boost REQUIRED COMPONENTS log log_setup stacktrace_windbg)
  target_link_libraries(plugin_segfault PRIVATE Boost::log Boost::log_setup
                                                Boost::stacktrace_windbg)
else()
  find_package(libbacktrace REQUIRED)
  target_compile_definitions(plugin_segfault
                             PRIVATE BOOST_STACKTRACE_USE_BACKTRACE)

  find_package(Boost REQUIRED COMPONENTS log log_setup stacktrace_backtrace)
  target_link_libraries(
    plugin_segfault
    PRIVATE Boost::log Boost::log_setup Boost::stacktrace_backtrace
            Boost::stacktrace_from_exception libbacktrace::libbacktrace dl)
  target_link_options(plugin_segfault PRIVATE -rdynamic)
endif()
