cmake_minimum_required(VERSION 3.30)
project(CHIP8 C)

set(CMAKE_C_STANDARD 11)
include_directories(lib)
add_compile_options(-Wall)

# https://wiki.libsdl.org/SDL3/README/cmake
add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

add_executable(CHIP8
    src/main.c
    src/chip8.c
)

target_link_libraries(CHIP8 PRIVATE SDL3::SDL3)

if(WIN32)
    # Copy DLL into build directory
    add_custom_command(
        TARGET CHIP8 POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:CHIP8>
        VERBATIM
    )
endif()
