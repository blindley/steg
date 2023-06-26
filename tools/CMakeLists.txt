cmake_minimum_required(VERSION 3.26)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(tools)

add_executable(refgen
    refgen.cpp
    ../src/image.cpp
    ../src/stb/stb.cpp
)

add_executable(crop
    crop.cpp
    ../src/image.cpp
    ../src/stb/stb.cpp
)

add_executable(red
    red.cpp
    ../src/image.cpp
    ../src/bcps.cpp
    ../src/stb/stb.cpp
)

add_executable(demolish
    demolish.cpp
    ../src/image.cpp
    ../src/bcps.cpp
    ../src/stb/stb.cpp
)
