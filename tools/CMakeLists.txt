cmake_minimum_required(VERSION 3.26)

project(tools)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    randfile
    randfile.cpp
)

add_executable(
    colorgram
    colorgram.cpp
    ../src/image.cpp
    ../src/stb/stb.cpp
    ../src/utility.cpp
)
