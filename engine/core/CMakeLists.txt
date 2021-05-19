cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE CORE_HEADERS "include/*.h")
file(GLOB_RECURSE CORE_SOURCES "source/*.cpp" "source/*.h")
file(GLOB_RECURSE CORE_RESOURCES "resource/*.h" "resource/*.rc")
add_library(core STATIC ${CORE_HEADERS} ${CORE_SOURCES} ${CORE_RESOURCES})

source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}/include/core"
    PREFIX "Header Files"
    FILES ${CORE_HEADERS}
)

source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}/source/core"
    PREFIX "Source Files"
    FILES ${CORE_SOURCES}
)

set_target_properties(core PROPERTIES FOLDER "engine")

target_include_directories(core PUBLIC "include")
target_include_directories(core PRIVATE "resource")
target_include_directories(core PRIVATE "source")

target_link_libraries(core PRIVATE debug_)
target_link_libraries(core PRIVATE Shlwapi.lib)

# Force all executables that link against "core" to load its resources too.
target_link_options(core PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/core.dir/$<CONFIG>/resource.res")

source_group("Resource Files" FILES ${CORE_RESOURCES})
