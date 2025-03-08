#include "chip8.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> /* For memset & memcpy */

/* Memory addresses */
#define MEM_PC_START 0x200
#define MEM_SP_START 0x1FE

/* Stack & register operations */
#define STACK_PUSH()       do { chip->sp -= 2; } while (0)
#define STACK_POP()        do { chip->sp += 2; } while (0)
#define JUMP_ADDR(addr)    do { chip->pc = (((addr) & 0x0FFF) - 2); } while (0)
#define NEXT_OPCODE()      do { chip->pc += 2; } while (0)
#define SKIP_NEXT_OPCODE() NEXT_OPCODE()

/* VM handlers */
#define OPCODE_HANDLER(op) static inline void VM_OpCode_Handler_##op(Chip8 *chip, const u16 opcode)

/* Shorthands */
typedef uint8_t u8;
typedef uint16_t u16;

/* Font sprites */
const static u8 FONT_LIST[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

/**
 * 0nnn: Jump to native assembler subroutine at 0xNNN (unused opcode).
 */
OPCODE_HANDLER(dw)
{
    chip->state |= ST_ERROR;
}

/**
 * 00E0: Clear the screen.
 */
OPCODE_HANDLER(cls)
{
    memset(chip->dp, 0, sizeof(chip->dp));
    chip->state |= ST_REDRAW;
}

/**
 * 00EE: Return from subroutine to address pulled from stack.
 */
OPCODE_HANDLER(ret)
{
    STACK_POP();
    u8 hi = chip->mem[chip->sp];
    u8 lo = chip->mem[chip->sp + 1];
    JUMP_ADDR((hi << 8) | lo);
}

/**
 * 1nnn: Jump to address NNN.
 */
OPCODE_HANDLER(jp)
{
    JUMP_ADDR(opcode);
}

/**
 * 2nnn: Push return address onto stack and call subroutine at address NNN.
 */
OPCODE_HANDLER(call)
{
    NEXT_OPCODE();
    u8 hi = (chip->pc & 0xFF00) >> 8;
    u8 lo = chip->pc  & 0x00FF;
    chip->mem[chip->sp] = hi;
    chip->mem[chip->sp + 1] = lo;
    STACK_PUSH();
    JUMP_ADDR(opcode);
}

/**
 * 3xnn: Skip next opcode if Vx == NN.
 */
OPCODE_HANDLER(sei)
{
    u8 x = (opcode & 0x0F00) >> 8;
    u8 nn = opcode & 0x00FF;

    if (chip->V[x] == nn) {
        SKIP_NEXT_OPCODE();
    }
}

/**
 * 4xnn: Skip next opcode if Vx != NN.
 */
OPCODE_HANDLER(snei)
{
    u8 x = (opcode & 0x0F00) >> 8;
    u8 nn = opcode & 0x00FF;

    if (chip->V[x] != nn) {
        SKIP_NEXT_OPCODE();
    }
}

/**
 * 5xy0: Skip next opcode if Vx == Vy.
 */
OPCODE_HANDLER(se)
{
    u8 x = (opcode & 0x0F00) >> 8;
    u8 y = (opcode & 0x00F0) >> 4;

    if (chip->V[x] == chip->V[y]) {
        SKIP_NEXT_OPCODE();
    }
}

/**
 * 6xnn: Set Vx to NN.
 */
OPCODE_HANDLER(ldi)
{
    u8 x = (opcode & 0x0F00) >> 8;
    u8 nn = opcode & 0x00FF;
    chip->V[x] = nn;
}

/**
 * 7xnn: Add NN to Vx.
 */
OPCODE_HANDLER(addi)
{
    u8 x = (opcode & 0x0F00) >> 8;
    u8 nn = opcode & 0x00FF;
    chip->V[x] += nn;
}

void chip8_init(Chip8 *chip)
{
    memset(chip, 0, sizeof(Chip8));
    /* Copy font to the unused region of memory (0x000 - 0x1FF) */
    memcpy(chip->mem, FONT_LIST, sizeof(FONT_LIST));
    chip->pc = MEM_PC_START;
    chip->sp = MEM_SP_START;
    chip->state = ST_RUNNING;
}

void chip8_step(Chip8 *chip)
{
    u8 hi = chip->mem[chip->pc];
    u8 lo = chip->mem[chip->pc + 1];
    u16 opcode = (hi << 8) | lo;

    /* Switch machine goes here */

    NEXT_OPCODE();
}
