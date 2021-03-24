cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE PHYSICS_EXAMPLE_SOURCES "source/*.cpp" "source/*.h")

add_executable(physics_example WIN32 ${PHYSICS_EXAMPLE_SOURCES})
set_target_properties(physics_example PROPERTIES FOLDER "examples")
