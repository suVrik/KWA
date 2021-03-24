cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE PHYSICS_SOURCES "source/*.cpp" "source/*.h")
file(GLOB_RECURSE PHYSICS_HEADERS "include/*.h")

add_library(physics STATIC ${PHYSICS_SOURCES} ${PHYSICS_HEADERS})
set_target_properties(physics PROPERTIES FOLDER "engine")
