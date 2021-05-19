cmake_minimum_required(VERSION 3.20)

find_package(SDL2 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

file(GLOB_RECURSE RENDER_HEADERS "include/*.h")
file(GLOB_RECURSE RENDER_SOURCES "source/*.cpp" "source/*.h")
add_library(render STATIC ${RENDER_HEADERS} ${RENDER_SOURCES})

source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}/include/render"
    PREFIX "Header Files"
    FILES ${RENDER_HEADERS}
)

source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}/source/render"
    PREFIX "Source Files"
    FILES ${RENDER_SOURCES}
)

set_target_properties(render PROPERTIES FOLDER "engine")

target_include_directories(render PUBLIC "include")
target_include_directories(render PRIVATE "source")

target_link_libraries(render PRIVATE debug_)
target_link_libraries(render PRIVATE system)
target_link_libraries(render PRIVATE SDL2::SDL2)
target_link_libraries(render PRIVATE Vulkan::Vulkan)
target_link_libraries(render PRIVATE spirv-reflect)
