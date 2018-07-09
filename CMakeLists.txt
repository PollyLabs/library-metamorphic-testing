cmake_minimum_required(VERSION 3.8)
project(LibMetaTesting)

set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(LIB_DIR ${CMAKE_SOURCE_DIR}/libs)

include_directories("${INCLUDE_DIR}")
link_directories("${LIB_DIR}")
file(GLOB SOURCES "src/*.cpp")

add_executable(isl_tester ${SOURCES})
target_link_libraries(isl_tester fmt)
target_link_libraries(isl_tester yaml-cpp)
target_link_libraries(isl_tester isl)
