cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE SPIRV_REFLECT_SOURCES "source/*.c" "source/*.h" "include/*.h")
add_library(spirv-reflect STATIC ${SPIRV_REFLECT_SOURCES})

set_target_properties(spirv-reflect PROPERTIES FOLDER "thirdparty")

target_include_directories(spirv-reflect PUBLIC "include")
target_include_directories(spirv-reflect PRIVATE "source")
