cmake_minimum_required(VERSION 3.8)
project(LibMetaTesting)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_COMPILER clang++)
#set(CMAKE_CXX_FLAGS -fsanitize=address)
set (CMAKE_CXX_STANDARD_LIBRARIES "-lboost_serialization")

set(CEREAL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/cereal/include)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(LIB_DIR ${CMAKE_SOURCE_DIR}/libs)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory("./third_party/fmt")
add_subdirectory("./third_party/yaml-cpp")

#find_package(LLVM REQUIRED CONFIG)
#find_package(Clang REQUIRED)
#include_directories(${LLVM_INCLUDE_DIRS})
#add_definitions(${LLVM_DEFINITIONS})

include_directories("${INCLUDE_DIR}"
		    "${CEREAL_INCLUDE_DIR}"	
		)
link_directories("${LIB_DIR}"
		)
link_directories("/usr/local_lib")
file(GLOB SOURCES "src/*.cpp")

#add_executable(test_emitter ${SOURCES})
add_executable(test_emitter
    ${SRC_DIR}/api_elements.cpp
    ${SRC_DIR}/api_fuzzer.cpp
    ${SRC_DIR}/reduction.cpp
    ${SRC_DIR}/set_meta_tester.cpp
    ${SRC_DIR}/test_emitter.cpp
    )
target_link_libraries(test_emitter fmt)
target_link_libraries(test_emitter yaml-cpp)

add_library(metalib_fuzz SHARED
    ${SRC_DIR}/api_elements.cpp
    ${SRC_DIR}/api_fuzzer.cpp
    ${SRC_DIR}/reduction.cpp
    ${SRC_DIR}/set_meta_tester.cpp
    ${SRC_DIR}/clang_interface.cpp
    ${SRC_DIR}/test_emitter.cpp
    )
