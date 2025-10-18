cmake_minimum_required(VERSION 3.18)

# Set the platform to C64 BEFORE the project() call
set(LLVM_MOS_PLATFORM c64)

# Find the LLVM-MOS SDK package
# Note: If LLVM-MOS is not in your PATH, you'll need to set CMAKE_PREFIX_PATH
find_package(llvm-mos-sdk REQUIRED)

# Project definition
project(whisper64 C)

# Add include directory
include_directories(${CMAKE_SOURCE_DIR}/include)

# Collect all source files from src directory
set(SOURCES
    src/main.c
    src/editor_state.c
    src/screen.c
    src/editor.c
    src/file_ops.c
    src/search.c
    src/clipboard.c
    src/basic.c
    src/help.c
    src/undo.c
    src/goto.c
    src/mouse.c
)

# Add the executable with all source files
add_executable(whisper64.prg ${SOURCES})

# Optimization flags for size 
target_compile_options(whisper64.prg PRIVATE -Os)

# Optional: Add custom target to show project structure
add_custom_target(show_structure
    COMMAND ${CMAKE_COMMAND} -E echo "Project Structure:"
    COMMAND ${CMAKE_COMMAND} -E echo "  include/ - Header files"
    COMMAND ${CMAKE_COMMAND} -E echo "  src/     - Source files"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)