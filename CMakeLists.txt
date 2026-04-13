cmake_minimum_required(VERSION 3.18)

# Set the platform to C64 BEFORE the project() call
set(LLVM_MOS_PLATFORM c64)

# Find the LLVM-MOS SDK package
# Set CMAKE_PREFIX_PATH if llvm-mos is not in your PATH:
#   cmake -DCMAKE_PREFIX_PATH=~/llvm-mos ..
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
    src/reu.c
    src/screen80.c
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
target_compile_options(whisper64.prg PRIVATE -Os -ffunction-sections)

target_link_options(whisper64.prg PRIVATE -Wl,--gc-sections -Wl,-s -Wl,-Map=whisper64.map)

# Create blank REU image file (512KB) if it doesn't exist
set(REU_IMAGE "${CMAKE_SOURCE_DIR}/whisper64.reu")
set(REU_SIZE_KB 512)
add_custom_command(
    OUTPUT ${REU_IMAGE}
    COMMAND dd if=/dev/zero of=${REU_IMAGE} bs=1024 count=${REU_SIZE_KB} 2>/dev/null
    COMMENT "Creating ${REU_SIZE_KB}KB REU image"
)
add_custom_target(reu_image DEPENDS ${REU_IMAGE})
add_dependencies(whisper64.prg reu_image)
