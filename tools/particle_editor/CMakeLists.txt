cmake_minimum_required(VERSION 3.19)

file(GLOB_RECURSE PARTICLE_EDITOR_SOURCES "source/*.cpp" "source/*.h")

add_executable(particle_editor WIN32 ${PARTICLE_EDITOR_SOURCES})
set_target_properties(particle_editor PROPERTIES FOLDER "tools")
