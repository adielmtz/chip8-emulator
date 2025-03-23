# CHIP-8 Emulator
An emulator for the COSMAC VIP CHIP-8 architecture.
Tested using ROMs from this [repo.](https://github.com/kripod/chip8-roms)

## Build
### Dependencies
* C compiler (gcc, clang, msvc, etc)
* CMake
* SDL3

### Steps
1. `git clone https://github.com/adielmtz/chip8-emulator && cd chip8-emulator`
2. `git clone https://github.com/libsdl-org/SDL vendor/SDL`
3. `cmake CMakeLists.txt`
4. `make`

## Architecture
This implementation is based in the COSMAC VIP hardware. An `opcode` is a 16-bit unsigned integer representing a CHIP-8 instruction (or command).


# References
https://chip8.gulrak.net/
https://en.wikipedia.org/wiki/CHIP-8
https://github.com/cookerlyk/Chip8
