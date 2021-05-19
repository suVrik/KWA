cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE render_example_sources "source/*.cpp" "source/*.h")
add_executable(render_example WIN32 ${render_example_sources})

set_target_properties(render_example PROPERTIES FOLDER "examples")

target_add_resources(render_example)

target_link_libraries(render_example PRIVATE core)
target_link_libraries(render_example PRIVATE system)
target_link_libraries(render_example PRIVATE render)
