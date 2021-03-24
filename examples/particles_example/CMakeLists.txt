cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE PARTICLES_EXAMPLE_SOURCES "source/*.cpp" "source/*.h")

add_executable(particles_example WIN32 ${PARTICLES_EXAMPLE_SOURCES})
set_target_properties(particles_example PROPERTIES FOLDER "examples")
