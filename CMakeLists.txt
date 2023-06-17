cmake_minimum_required(VERSION 3.26)

project(steg)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip 
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(steg
    src/args.cpp
    src/image.cpp
    src/main.cpp
    src/steg.cpp
    src/lsb.cpp
    src/bcps.cpp
    src/stb/stb.cpp
)

add_subdirectory(tools)

enable_testing()

add_executable(
    steg_test
    src/test_main.cpp
)

target_link_libraries(
    steg_test
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(steg_test)
