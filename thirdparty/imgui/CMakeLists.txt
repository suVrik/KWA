cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE IMGUI_SOURCES "source/*.cpp" "source/*.h" "include/*.h")
add_library(imgui STATIC ${IMGUI_SOURCES})

set_target_properties(imgui PROPERTIES FOLDER "thirdparty")

target_include_directories(imgui PUBLIC "include")
target_include_directories(imgui PRIVATE "source")
