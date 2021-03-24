cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE LEVEL_EDITOR_SOURCES "source/*.cpp" "source/*.h")

add_executable(level_editor WIN32 ${LEVEL_EDITOR_SOURCES})
set_target_properties(level_editor PROPERTIES FOLDER "tools")
