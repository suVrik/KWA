cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE MEMORY_SOURCES "source/*.cpp" "source/*.h" "include/*.h")
add_library(memory STATIC ${MEMORY_SOURCES})

set_target_properties(memory PROPERTIES FOLDER "engine")

target_include_directories(memory PUBLIC "include")
target_include_directories(memory PRIVATE "source")

target_link_libraries(memory PRIVATE debug_)
