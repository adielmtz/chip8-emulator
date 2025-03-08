#pragma once

#include <stdint.h>

#define DISPLAY_WIDTH  64
#define DISPLAY_HEIGHT 32

#define ST_RUNNING (1 << 0)
#define ST_PAUSED  (1 << 1)
#define ST_REDRAW  (1 << 2)
#define ST_ERROR   (1 << 3)

typedef struct Chip8
{
    uint8_t mem[4096];
    uint8_t dp[DISPLAY_WIDTH * DISPLAY_HEIGHT];
    uint8_t V[16];

    // Keyboard
    uint8_t kb[16];

    // Pointers
    uint16_t pc;
    uint16_t sp;
    uint16_t I;

    // Extra
    uint16_t state;

    // Timers
    uint8_t delay;
    uint8_t sound;
} Chip8;

void chip8_init(Chip8 *chip);
void chip8_load(Chip8 *chip, const char *rom);
void chip8_step(Chip8 *chip);
