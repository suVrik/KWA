cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE MESH_VIEWER_SOURCES "source/*.cpp" "source/*.h")

add_executable(mesh_viewer WIN32 ${MESH_VIEWER_SOURCES})
set_target_properties(mesh_viewer PROPERTIES FOLDER "tools")
