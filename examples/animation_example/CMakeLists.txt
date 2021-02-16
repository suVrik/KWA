cmake_minimum_required(VERSION 3.19)

file(GLOB_RECURSE ANIMATION_EXAMPLE_SOURCES "source/*.cpp" "source/*.h")

add_executable(animation_example WIN32 ${ANIMATION_EXAMPLE_SOURCES})
set_target_properties(animation_example PROPERTIES FOLDER "examples")
