cmake_minimum_required(VERSION 3.19)

file(GLOB_RECURSE OVERLAYS_SOURCES "source/*.cpp" "source/*.h")
file(GLOB_RECURSE OVERLAYS_HEADERS "include/*.h")

add_library(overlays STATIC ${OVERLAYS_SOURCES} ${OVERLAYS_HEADERS})
set_target_properties(overlays PROPERTIES FOLDER "engine")
