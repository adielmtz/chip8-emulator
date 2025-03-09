#include "chip8.h"
#include "graphics.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <SDL3/SDL.h>

#define VCPU_FREQ_HZ (10 / 1000)
#define TIME_FREQ_HZ (60 / 1000)

const static uint8_t KEYMAP[] = {
    SDLK_X, // 0
    SDLK_1, // 1
    SDLK_2, // 2
    SDLK_3, // 3
    SDLK_Q, // 4
    SDLK_W, // 5
    SDLK_E, // 6
    SDLK_A, // 7
    SDLK_S, // 8
    SDLK_D, // 9
    SDLK_Z, // A
    SDLK_C, // B
    SDLK_4, // C
    SDLK_R, // D
    SDLK_F, // E
    SDLK_V  // F
};

static void process_user_event(Chip8* chip, uint8_t *shutdown)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            *shutdown = 1;
            return;
        }

        if (event.type == SDL_EVENT_KEY_DOWN) {
            SDL_Keycode key = event.key.key;

            if (key == SDLK_ESCAPE) {
                chip->state &= ~ST_RUNNING;
                return;
            }

            for (int i = 0; i < 16; i++) {
                if (key == KEYMAP[i]) {
                    chip->kb[i] = 1;
                }
            }
        }

        if (event.type == SDL_EVENT_KEY_UP) {
            SDL_Keycode key = event.key.key;

            for (int i = 0; i < 16; i++) {
                if (key == KEYMAP[i]) {
                    chip->kb[i] = 0;
                }
            }
        }
    }
}

int main(void)
{
    Chip8 chip;
    chip8_init(&chip);
    chip8_load(&chip, "Cave.ch8");

    Graphics g;
    graphics_init(&g, DISPLAY_WIDTH, DISPLAY_HEIGHT, 20);

    // Seed random for rnd opcode in chip-8
    srand(time(NULL));

    uint64_t last_cpu_tick = SDL_GetTicks();
    uint64_t last_timer_tick = SDL_GetTicks();

    while (chip.state & ST_RUNNING) {
        uint64_t ticks = SDL_GetTicks();
        uint64_t delta = ticks - last_cpu_tick;

        if (delta >= VCPU_FREQ_HZ) {
            chip8_step(&chip);

            if (chip.state & ST_ERROR) {
                fprintf(stderr, "VM Error!\n");
            }

            if (chip.state & ST_REDRAW) {
                graphics_copy(&g, chip.dp);
                graphics_draw(&g);
                chip.state &= ~ST_REDRAW;
            }

            uint8_t shutdown = 0;
            process_user_event(&chip, &shutdown);

            if (shutdown) {
                break;
            }
        }

        if (delta >= TIME_FREQ_HZ) {
            if (chip.delay > 0) {
                chip.delay--;
            }

            if (chip.sound > 0) {
                chip.sound--;
            }
        }

        SDL_Delay(1);
    }

    graphics_shutdown(&g);
    return 0;
}
