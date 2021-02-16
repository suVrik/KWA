cmake_minimum_required(VERSION 3.19)

file(GLOB_RECURSE BLEND_TREE_EDITOR_SOURCES "source/*.cpp" "source/*.h")

add_executable(blend_tree_editor WIN32 ${BLEND_TREE_EDITOR_SOURCES})
set_target_properties(blend_tree_editor PROPERTIES FOLDER "tools")
