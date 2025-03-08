#include "chip8.h"

#include <stdio.h>
#include <stdlib.h> /* For rand */
#include <stdint.h>
#include <string.h> /* For memset & memcpy */

/* Memory addresses */
#define MEM_PC_START 0x200
#define MEM_SP_START 0x1FE

/* Stack & register operations */
#define STACK_PUSH()       do { chip->sp -= 2; } while (0)
#define STACK_POP()        do { chip->sp += 2; } while (0)
#define JUMP_ADDR(addr)    do { chip->pc = (((addr) & 0x0FFF) - 2); } while (0)
#define RETRY_OPCODE()     do { chip->pc -= 2; } while (0)
#define NEXT_OPCODE()      do { chip->pc += 2; } while (0)
#define SKIP_NEXT_OPCODE() NEXT_OPCODE()

/* VM handlers */
#define OPCODE_HANDLER(op) static inline void VM_OpCode_Handler_##op(Chip8 *chip, const u16 opcode)
#define OPCODE_EXECUTE(op) VM_OpCode_Handler_##op(chip, opcode)

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

/**
 * 8xy0: Set Vx to the value of Vy.
 */
OPCODE_HANDLER(mov)
{
    u8 x = (opcode & 0x0F00) >> 8;
    u8 y = (opcode & 0x00F0) >> 4;
    chip->V[x] = chip->V[y];
}

/**
 * 8xy1: Set Vx to the result of bitwise Vx OR Vy. Side-effect: resets Vf.
 */
OPCODE_HANDLER(or)
{
    u8 x = (opcode & 0x0F00) >> 8;
    u8 y = (opcode & 0x00F0) >> 4;
    chip->V[x] |= chip->V[y];
    chip->V[0xF] = 0;
}

/**
 * 8xy2: Set Vx to the result of bitwise Vx AND Vy. Side-effect: resets Vf.
 */
OPCODE_HANDLER(and)
{
    u8 x = (opcode & 0x0F00) >> 8;
    u8 y = (opcode & 0x00F0) >> 4;
    chip->V[x] &= chip->V[y];
    chip->V[0xF] = 0;
}

/**
 * 8xy3: Set Vx to the result of bitwise Vx XOR Vy. Side-effect: resets Vf.
 */
OPCODE_HANDLER(xor)
{
    u8 x = (opcode & 0x0F00) >> 8;
    u8 y = (opcode & 0x00F0) >> 4;
    chip->V[x] ^= chip->V[y];
    chip->V[0xF] = 0;
}

/**
 * 8xy4: Add Vy to Vx. Vf is set to 1 if an overflow happened, to 0 if not.
 */
OPCODE_HANDLER(add)
{
    u8 x = (opcode & 0x0F00) >> 8;
    u8 y = (opcode & 0x00F0) >> 4;
    u16 z = chip->V[x] + chip->V[y];
    chip->V[0xF] = z > 0xFF;
    chip->V[x] = z & 0xFF;
}

/**
 * 8xy5: Subtract Vy from Vx. Vf is set to 0 if an underflow happened, to 1 if not.
 */
OPCODE_HANDLER(sub)
{
    u8 x = (opcode & 0x0F00) >> 8;
    u8 y = (opcode & 0x00F0) >> 4;
    u16 z = chip->V[x] - chip->V[y];
    chip->V[0xF] = z <= chip->V[x];
    chip->V[x] = z & 0xFF;
}

/**
 * 8xy6: Shift Vx one bit to the right. Set Vf to the bit shifted out.
 */
OPCODE_HANDLER(shr)
{
    u8 x = (opcode & 0x0F00) >> 8;
    chip->V[0xF] = chip->V[x] & 0x01;
    chip->V[x] >>= 1;
}

/**
 * 8xy7: Subtract Vx from Vy. Vf is set to 0 if an underflow happened, to 1 if not.
 */
OPCODE_HANDLER(subn)
{
    u8 x = (opcode & 0x0F00) >> 8;
    u8 y = (opcode & 0x00F0) >> 4;
    u16 z = chip->V[y] - chip->V[x];
    chip->V[0xF] = z <= chip->V[y];
    chip->V[x] = z & 0xFF;
}

/**
 * 8xyE: Shift Vx one bit to the left. Set Vf to the bit shifted out.
 */
OPCODE_HANDLER(shl)
{
    u8 x = (opcode & 0x0F00) >> 8;
    chip->V[0xF] = chip->V[x] & 0x01;
    chip->V[x] <<= 1;
}

/**
 * 9xy0: Skip next opcode if Vx != Vy.
 */
OPCODE_HANDLER(sne)
{
    u8 x = (opcode & 0x0F00) >> 8;
    u8 y = (opcode & 0x00F0) >> 4;

    if (chip->V[x] != chip->V[y]) {
        SKIP_NEXT_OPCODE();
    }
}

/**
 * Annn: Set I to the address NNN.
 */
OPCODE_HANDLER(mvi)
{
    chip->I = opcode & 0x0FFF;
}

/**
 * Bnnn: Jump to the address NNN + V0.
 */
OPCODE_HANDLER(jpi)
{
    u16 nnn = opcode & 0x0FFF;
    JUMP_ADDR(nnn + chip->V[0]);
}

/**
 * Cxnn: Set Vx to the result of a bitwise AND operation on a random number and NN.
 */
OPCODE_HANDLER(rnd)
{
    u8 x = (opcode & 0x0F00) >> 8;
    u8 nn = opcode & 0x00FF;
    chip->V[x] = rand() & nn;
}

/**
 * Dxyn: Draw a sprite at coordinate (Vx, Vy) that has a width of 8 pixels and a height of N pixels.
 * Each row of 8 pixels is read as bit-coded starting from memory location I.
 */
OPCODE_HANDLER(drw)
{
    u8 x = (opcode & 0x0F00) >> 8;
    u8 y = (opcode & 0x00F0) >> 4;
    u8 n = opcode & 0x000F;

    /* Get top-left corner coordinates */
    u8 cx = chip->V[x];
    u8 cy = chip->V[y];

    /* Reset Vf */
    chip->V[0xF] = 0;

    /* TODO: Implement logic here... */

    /* Instruct graphics layer to redraw the screen */
    chip->state |= ST_REDRAW;
}

/**
 * Ex9E: Skip next opcode if key in the lower 4 bits of Vx is pressed.
 */
OPCODE_HANDLER(skp)
{
    u8 x = (opcode & 0x0F00) >> 8;
    u8 n = chip->V[x] & 0x0F;

    if (chip->kb[n]) {
        SKIP_NEXT_OPCODE();
    }
}

/**
 * ExA1: Skip next opcode if key in the lower 4 bits of Vx is NOT pressed.
 */
OPCODE_HANDLER(sknp)
{
    u8 x = (opcode & 0x0F00) >> 8;
    u8 n = chip->V[x] & 0x0F;

    if (!chip->kb[n]) {
        SKIP_NEXT_OPCODE();
    }
}

/**
 * Fx07: Set Vx to the value of the delay timer.
 */
OPCODE_HANDLER(stdt)
{
    u8 x = (opcode & 0x0F00) >> 8;
    chip->V[x] = chip->delay;
}

/**
 * Fx0A: Wait for a key to be pressed and released, and set Vx to it.
 * Blocking opcode. All instructions are halted until next key event.
 * Delay and sound timers must continue processing.
 */
OPCODE_HANDLER(stkp)
{
    u8 x = (opcode & 0x0F00) >> 8;

    for (u8 i = 0; i < 16; i++) {
        if (chip->kb[i]) {
            chip->V[x] = i;
            return;
        }
    }

    RETRY_OPCODE();
}

/**
 * Fx15: Set delay timer to Vx.
 */
OPCODE_HANDLER(lddt)
{
    u8 x = (opcode & 0x0F00) >> 8;
    chip->delay = chip->V[x];
}

/**
 * Fx18: Set sound timer to Vx.
 */
OPCODE_HANDLER(ldst)
{
    u8 x = (opcode & 0x0F00) >> 8;
    chip->sound = chip->V[x];
}

/**
 * Fx1E: Add Vx to I.
 */
OPCODE_HANDLER(stoi)
{
    u8 x = (opcode & 0x0F00) >> 8;
    chip->I += chip->V[x];
}

/**
 * Fx29: Set I to the 5-line high hex sprite for the lowest nibble in Vx.
 */
OPCODE_HANDLER(sprt)
{
    u8 x = (opcode & 0x0F00) >> 8;
    chip->I = chip->V[x] * 5;
}

/**
 * Fx33: Write the value of Vx as BCD value at the addresses I, I+1, and I+2.
 */
OPCODE_HANDLER(bcd)
{
    u8 x = (opcode & 0x0F00) >> 8;
    chip->mem[chip->I + 0] = chip->V[x] / 100;
    chip->mem[chip->I + 1] = (chip->V[x] / 10) % 10;
    chip->mem[chip->I + 2] = chip->V[x] % 10;
}

/**
 * Fx55: Write the registers from V0 to Vx at memory address pointed to by I. I is incremented by X+1.
 */
OPCODE_HANDLER(push)
{
    u8 x = (opcode & 0x0F00) >> 8;

    for (u8 i = 0; i <= x; i++) {
        chip->mem[chip->I + i] = chip->V[i];
    }

    chip->I += x + 1;
}

/**
 * Fx65: Read the bytes from memory address pointed to by I into the registers V0 to Vx. I is incremented by X+1.
 */
OPCODE_HANDLER(pop)
{
    u8 x = (opcode & 0x0F00) >> 8;

    for (u8 i = 0; i <= x; i++) {
        chip->V[i] = chip->mem[chip->I + i];
    }

    chip->I += x + 1;
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

void chip8_load(Chip8 *chip, const char *rom)
{
    u8 buffer[0x0CA0];
    memset(buffer, 0, sizeof(buffer));

    FILE *handle = fopen(rom, "rb");
    if (!handle) {
        /* TODO: Implement proper error handling */
        return;
    }

    size_t bytes = fread(buffer, sizeof(u8), sizeof(buffer), handle);
    fclose(handle);

    /* Copy rom at the starting address */
    memcpy(&chip->mem[MEM_PC_START], buffer, bytes);
}

void chip8_step(Chip8 *chip)
{
    u8 hi = chip->mem[chip->pc];
    u8 lo = chip->mem[chip->pc + 1];
    u16 opcode = (hi << 8) | lo;

    switch (hi & 0xF0) {
        case 0x00: {
            switch (lo) {
                case 0xE0: OPCODE_EXECUTE(cls); break;
                case 0xEE: OPCODE_EXECUTE(ret); break;
                default:   OPCODE_EXECUTE(dw);  break;
            }
            break;
        }
        case 0x10: OPCODE_EXECUTE(jp);   break;
        case 0x20: OPCODE_EXECUTE(call); break;
        case 0x30: OPCODE_EXECUTE(sei);  break;
        case 0x40: OPCODE_EXECUTE(snei); break;
        case 0x50: OPCODE_EXECUTE(se);   break;
        case 0x60: OPCODE_EXECUTE(ldi);  break;
        case 0x70: OPCODE_EXECUTE(addi); break;
        case 0x80: {
            switch (lo & 0x0F) {
                case 0x00: OPCODE_EXECUTE(mov);  break;
                case 0x01: OPCODE_EXECUTE(or);   break;
                case 0x02: OPCODE_EXECUTE(and);  break;
                case 0x03: OPCODE_EXECUTE(xor);  break;
                case 0x04: OPCODE_EXECUTE(add);  break;
                case 0x05: OPCODE_EXECUTE(sub);  break;
                case 0x06: OPCODE_EXECUTE(shr);  break;
                case 0x07: OPCODE_EXECUTE(subn); break;
                case 0x0E: OPCODE_EXECUTE(shl);  break;
            }
            break;
        }
        case 0x90: OPCODE_EXECUTE(sne); break;
        case 0xA0: OPCODE_EXECUTE(mvi); break;
        case 0xB0: OPCODE_EXECUTE(jpi); break;
        case 0xC0: OPCODE_EXECUTE(rnd); break;
        case 0xD0: OPCODE_EXECUTE(drw); break;
        case 0xE0: {
            switch (lo) {
                case 0x9E: OPCODE_EXECUTE(skp);  break;
                case 0xA1: OPCODE_EXECUTE(sknp); break;
            }
            break;
        }
        case 0xF0: {
            switch (lo) {
                case 0x07: OPCODE_EXECUTE(stdt); break;
                case 0x0A: OPCODE_EXECUTE(stkp); break;
                case 0x15: OPCODE_EXECUTE(lddt); break;
                case 0x18: OPCODE_EXECUTE(ldst); break;
                case 0x1E: OPCODE_EXECUTE(stoi); break;
                case 0x29: OPCODE_EXECUTE(sprt); break;
                case 0x33: OPCODE_EXECUTE(bcd);  break;
                case 0x55: OPCODE_EXECUTE(push); break;
                case 0x65: OPCODE_EXECUTE(pop);  break;
            }
            break;
        }
    }

    NEXT_OPCODE();
}
