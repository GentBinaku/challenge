add_executable(challenge
	src/main.cpp
)

set_target_properties(challenge PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(challenge
	PRIVATE
		${PROJECT_SOURCE_DIR}/plugin/include
)
