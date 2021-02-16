cmake_minimum_required(VERSION 3.19)

file(GLOB_RECURSE NETWORK_SOURCES "source/*.cpp" "source/*.h")
file(GLOB_RECURSE NETWORK_HEADERS "include/*.h")

add_library(network STATIC ${NETWORK_SOURCES} ${NETWORK_HEADERS})
set_target_properties(network PROPERTIES FOLDER "engine")
