cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE SOUND_EXAMPLE_SOURCES "source/*.cpp" "source/*.h")

add_executable(sound_example WIN32 ${SOUND_EXAMPLE_SOURCES})
set_target_properties(sound_example PROPERTIES FOLDER "examples")
