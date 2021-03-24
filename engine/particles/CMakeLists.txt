cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE PARTICLES_SOURCES "source/*.cpp" "source/*.h")
file(GLOB_RECURSE PARTICLES_HEADERS "include/*.h")

add_library(particles STATIC ${PARTICLES_SOURCES} ${PARTICLES_HEADERS})
set_target_properties(particles PROPERTIES FOLDER "engine")
