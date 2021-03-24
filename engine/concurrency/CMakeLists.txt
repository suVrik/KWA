cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE CONCURRENCY_SOURCES "source/*.cpp" "source/*.h" "include/*.h")
add_library(concurrency STATIC ${CONCURRENCY_SOURCES})

set_target_properties(concurrency PROPERTIES FOLDER "engine")

target_include_directories(concurrency PUBLIC "include")
target_include_directories(concurrency PRIVATE "source")
