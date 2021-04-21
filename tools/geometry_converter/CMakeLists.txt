cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE GEOMETRY_CONVERTER_SOURCES "source/*.cpp" "source/*.h")

add_executable(geometry_converter ${GEOMETRY_CONVERTER_SOURCES})
set_target_properties(geometry_converter PROPERTIES FOLDER "tools")

target_link_libraries(geometry_converter PRIVATE core)
target_link_libraries(geometry_converter PRIVATE tinygltf)
