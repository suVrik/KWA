cmake_minimum_required(VERSION 3.20)

find_package(PhysX CONFIG REQUIRED)

file(GLOB_RECURSE PHYSICS_HEADERS "include/*.h")
file(GLOB_RECURSE PHYSICS_SOURCES "source/*.cpp" "source/*.h")
add_library(physics STATIC ${PHYSICS_HEADERS} ${PHYSICS_SOURCES})

source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}/include/physics"
    PREFIX "Header Files"
    FILES ${PHYSICS_HEADERS}
)

source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}/source/physics"
    PREFIX "Source Files"
    FILES ${PHYSICS_SOURCES}
)

set_target_properties(physics PROPERTIES FOLDER "engine")

target_include_directories(physics PUBLIC "include")
target_include_directories(physics PRIVATE "source")

target_link_libraries(physics PUBLIC core)
target_link_libraries(physics PRIVATE PhysX::PhysX)
