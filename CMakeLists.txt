cmake_minimum_required(VERSION 3.8)
project(LibMetaTesting)
set(CMAKE_BUILD_TYPE Debug)

set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(LIB_DIR ${CMAKE_SOURCE_DIR}/libs)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories("${INCLUDE_DIR}")
link_directories("${LIB_DIR}")
file(GLOB SOURCES "src/*.cpp")

add_executable(test_emitter ${SOURCES})
target_link_libraries(test_emitter fmt)
target_link_libraries(test_emitter yaml-cpp)
target_link_libraries(test_emitter isl)
