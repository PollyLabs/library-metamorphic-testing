cmake_minimum_required(VERSION 3.13)
project(LibMetaTesting)
set(CMAKE_CXX_STANDARD 17)

if (${CMAKE_BUILD_TYPE} EQUAL Debug)
    add_compile_definitions(DEBUG)
endif()

set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(LIB_DIR ${PROJECT_SOURCE_DIR}/libs)
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_policy(SET CMP0077 NEW)
set(BUILD_SHARED_LIBS ON)
#set(YAML_BUILD_SHARED_LIBS ON)
add_subdirectory("./third_party/fmt")
add_subdirectory("./third_party/yaml-cpp")
add_subdirectory("./third_party/random")

include_directories("${INCLUDE_DIR}")
link_directories("${LIB_DIR}")
link_directories("/usr/local_lib")

add_library(metalib_fuzz SHARED
    ${SRC_DIR}/api_elements.cpp
    ${SRC_DIR}/api_fuzzer.cpp
    ${SRC_DIR}/set_meta_tester.cpp
    ${SRC_DIR}/clang_interface.cpp
    ${SRC_DIR}/test_emitter.cpp
    )
target_link_libraries(metalib_fuzz fmt)
target_link_libraries(metalib_fuzz yaml-cpp)
target_link_libraries(metalib_fuzz effolkronium_random)

if (METALIB_LIB_ONLY)
    return()
endif()

add_executable(test_emitter
    ${SRC_DIR}/api_elements.cpp
    ${SRC_DIR}/api_fuzzer.cpp
    ${SRC_DIR}/set_meta_tester.cpp
    ${SRC_DIR}/test_emitter.cpp
    )
target_link_libraries(test_emitter fmt)
target_link_libraries(test_emitter yaml-cpp)
