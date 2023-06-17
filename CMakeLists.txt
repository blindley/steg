cmake_minimum_required(VERSION 3.26)

# Set the project name.
project(steg)

# Add an executable.
add_executable(steg
    src/args.cpp
    src/image.cpp
    src/main.cpp
    src/steg.cpp
    src/lsb.cpp
    src/bcps.cpp
    src/stb/stb.cpp
)

# Specify the C++ standard.
set_target_properties(steg PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED True
)

add_subdirectory(tools)
