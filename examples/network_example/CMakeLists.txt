cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE NETWORK_EXAMPLE_SOURCES "source/*.cpp" "source/*.h")

add_executable(network_example WIN32 ${NETWORK_EXAMPLE_SOURCES})
set_target_properties(network_example PROPERTIES FOLDER "examples")
