cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE ANIMATION_VIEWER_SOURCES "source/*.cpp" "source/*.h")

add_executable(animation_viewer WIN32 ${ANIMATION_VIEWER_SOURCES})
set_target_properties(animation_viewer PROPERTIES FOLDER "tools")
