cmake_minimum_required(VERSION 3.30)
project(CHIP8 C)

set(CMAKE_C_STANDARD 11)
include_directories(lib)
add_compile_options(-Wall)

add_executable(CHIP8
    src/main.c
)
