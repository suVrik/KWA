cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE TINYGLTF_SOURCES "source/*.cpp" "source/*.hpp" "include/*.h")
add_library(tinygltf STATIC ${TINYGLTF_SOURCES})

set_target_properties(tinygltf PROPERTIES FOLDER "thirdparty")

target_include_directories(tinygltf PUBLIC "include")
target_include_directories(tinygltf PRIVATE "source")
