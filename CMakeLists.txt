##################################################################
# wasmtoolbox cmake build definition
# Copyright (c) 2023 Patrick Varilly
##################################################################

cmake_minimum_required(VERSION 3.17.5)

project(wasmtoolbox)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

# Set up common options
add_library(common INTERFACE)

# Treat all warnings as errors
target_compile_options(common INTERFACE -Wall -Wextra -Werror)
add_compile_options(-fno-omit-frame-pointer)

# Find required and optional libraries
set(ABSL_PROPAGATE_CXX_STD ON)
add_subdirectory(third-party/abseil-cpp EXCLUDE_FROM_ALL)

# Main Components
add_subdirectory(lib)

# Test Components
cmake_policy(SET CMP0135 NEW)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  )
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_subdirectory(tests)

# main executable
add_executable(wasmtoolbox wasmtoolbox.cpp)
target_link_libraries(wasmtoolbox lib)
