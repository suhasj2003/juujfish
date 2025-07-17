cmake_minimum_required(VERSION 3.15...4.0)

project(chessengine
    VERSION 0.2.2
    DESCRIPTION "High performance chess engine"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/include)
add_subdirectory(src/core)