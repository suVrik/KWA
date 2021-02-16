cmake_minimum_required(VERSION 3.19)

find_package(GTest CONFIG REQUIRED)

file(GLOB_RECURSE UNITTEST_SOURCES "source/*.cpp" "source/*.h")

add_executable(unittest ${UNITTEST_SOURCES})

target_link_libraries(unittest PRIVATE GTest::gmock GTest::gtest)
