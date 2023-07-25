cmake_minimum_required(VERSION 3.26)

project(steg)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(steg
    src/main.cpp
    src/args.cpp
    src/image.cpp
    src/bpcs.cpp
    src/message.cpp
    src/datachunk.cpp
    src/utility.cpp
    src/stb.cpp
)

enable_testing()

add_executable(
    steg_test
    src/image.cpp
    src/bpcs.cpp
    src/message.cpp
    src/datachunk.cpp
    src/utility.cpp
    src/stb.cpp
)

target_compile_definitions(steg_test PRIVATE STEG_TEST)

target_link_libraries(
    steg_test
    GTest::gtest_main
)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip 
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)
gtest_discover_tests(steg_test)


FetchContent_Declare(
    stb_image
    URL https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
    DOWNLOAD_NO_EXTRACT true
    DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/include
)

FetchContent_GetProperties(stb_image)
if (NOT stb_image_POPULATED)
    FetchContent_Populate(stb_image)
endif()

FetchContent_Declare(
    stb_image_write
    URL https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
    DOWNLOAD_NO_EXTRACT true
    DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/include
)

FetchContent_GetProperties(stb_image_write)
if (NOT stb_image_write_POPULATED)
    FetchContent_Populate(stb_image_write)
endif()

target_include_directories(steg PRIVATE ${CMAKE_BINARY_DIR}/include)
target_include_directories(steg_test PRIVATE ${CMAKE_BINARY_DIR}/include)
