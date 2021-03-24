cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE ANIMATION_SOURCES "source/*.cpp" "source/*.h")
file(GLOB_RECURSE ANIMATION_HEADERS "include/*.h")

add_library(animation STATIC ${ANIMATION_SOURCES} ${ANIMATION_HEADERS})
set_target_properties(animation PROPERTIES FOLDER "engine")
