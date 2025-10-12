cmake_minimum_required(VERSION 3.18)

# Set the platform to C64 BEFORE the project() call
set(LLVM_MOS_PLATFORM c64)

# Find the LLVM-MOS SDK package
# Note: If LLVM-MOS is not in your PATH, you'll need to set CMAKE_PREFIX_PATH
find_package(llvm-mos-sdk REQUIRED)

# Project definition
project(whisper64 C)

# Add the executable
add_executable(whisper64.prg whisper64.c)

# Optimization flags for size 
target_compile_options(whisper64.prg PRIVATE -Os)

