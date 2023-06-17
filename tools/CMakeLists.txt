cmake_minimum_required(VERSION 3.26)

# Set the project name.
project(refgen)

# Add an executable.
add_executable(refgen
    refgen.cpp
    ../src/image.cpp
    ../src/stb/stb.cpp
)

# Specify the C++ standard.
set_target_properties(refgen PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED True
)
