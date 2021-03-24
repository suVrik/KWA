cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE SOUND_SOURCES "source/*.cpp" "source/*.h")
file(GLOB_RECURSE SOUND_HEADERS "include/*.h")

add_library(sound STATIC ${SOUND_SOURCES} ${SOUND_HEADERS})
set_target_properties(sound PROPERTIES FOLDER "engine")
