# Minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Project name
project(cp-utils)

# Add the executable target
add_executable(testsuite testsuite/main.cpp)

# Set compiler flags
target_compile_options(testsuite PRIVATE -O3 -mavx2)

# Install the executable
install(TARGETS testsuite DESTINATION bin)
