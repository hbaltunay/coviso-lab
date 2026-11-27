cmake_minimum_required(VERSION 3.15)

project(cv VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_library(cv STATIC ${SOURCES})

target_include_directories(cv PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)

add_subdirectory(apps)