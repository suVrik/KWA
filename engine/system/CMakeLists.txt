cmake_minimum_required(VERSION 3.20)

find_package(SDL2 CONFIG REQUIRED)

file(GLOB_RECURSE SYSTEM_SOURCES "source/*.cpp" "source/*.h" "include/*.h")
add_library(system STATIC ${SYSTEM_SOURCES})

set_target_properties(system PROPERTIES FOLDER "engine")

target_include_directories(system PUBLIC "include")
target_include_directories(system PRIVATE "source")

target_link_libraries(system PUBLIC core)
target_link_libraries(system PRIVATE debug_)
target_link_libraries(system PRIVATE SDL2::SDL2)
