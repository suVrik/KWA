cmake_minimum_required(VERSION 3.19)

file(GLOB_RECURSE DEBUG_SOURCES "source/*.cpp" "source/*.h" "include/*.h")
file(GLOB_RECURSE DEBUG_RESOURCES "resource/*.h" "resource/*.rc")
add_library(debug STATIC ${DEBUG_SOURCES} ${DEBUG_RESOURCES})

# "debug" is a keyword in target_link_libraries command, so this library must be linked as "debug_".
add_library(debug_ ALIAS debug)

set_target_properties(debug PROPERTIES FOLDER "engine")

target_include_directories(debug PUBLIC "include")
target_include_directories(debug PRIVATE "resource")
target_include_directories(debug PRIVATE "source")

target_link_libraries(debug PRIVATE DbgHelp)

# Force all executables that link against "debug_" to load its resources too.
target_link_options(debug PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/debug.dir/$<CONFIG>/resource.res")

source_group("Resource Files" FILES ${DEBUG_RESOURCES})
