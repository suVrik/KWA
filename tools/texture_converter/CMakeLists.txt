cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE TEXTURE_CONVERTER_SOURCES "source/*.cpp" "source/*.h")

add_executable(texture_converter ${TEXTURE_CONVERTER_SOURCES})
set_target_properties(texture_converter PROPERTIES FOLDER "tools")

target_link_libraries(texture_converter PRIVATE core)
target_link_libraries(texture_converter PRIVATE render)
