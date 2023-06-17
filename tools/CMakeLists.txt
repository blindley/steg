cmake_minimum_required(VERSION 3.26)

project(refgen)

add_executable(refgen
    refgen.cpp
    ../src/image.cpp
    ../src/stb/stb.cpp
)

set_target_properties(refgen PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED True
)


project(crop)

add_executable(crop
    crop.cpp
    ../src/image.cpp
    ../src/stb/stb.cpp
)

set_target_properties(crop PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED True
)
