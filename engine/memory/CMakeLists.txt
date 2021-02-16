cmake_minimum_required(VERSION 3.19)

file(GLOB_RECURSE MEMORY_SOURCES "source/*.cpp" "source/*.h")
file(GLOB_RECURSE MEMORY_HEADERS "include/*.h")

add_library(memory STATIC ${MEMORY_SOURCES} ${MEMORY_HEADERS})
set_target_properties(memory PROPERTIES FOLDER "engine")
