cmake_minimum_required(VERSION 3.19)

find_package(SDL2 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

file(GLOB_RECURSE RENDER_SOURCES "source/*.cpp" "source/*.h" "include/*.h")
add_library(render STATIC ${RENDER_SOURCES})

set_target_properties(render PROPERTIES FOLDER "engine")

target_include_directories(render PUBLIC "include")
target_include_directories(render PRIVATE "source")

target_link_libraries(render PRIVATE debug_)
target_link_libraries(render PRIVATE core)
target_link_libraries(render PRIVATE system)
target_link_libraries(render PRIVATE SDL2::SDL2)
target_link_libraries(render PRIVATE Vulkan::Vulkan)
