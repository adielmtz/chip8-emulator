#include "graphics.h"

#include <stdlib.h> /* For malloc & free */
#include <stdint.h>
#include <string.h> /* For memset */

void graphics_init(Graphics *g, int w, int h, int scale)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS /*| SDL_INIT_AUDIO*/)) {
        /* TODO: Implement error checking */
        return;
    }

    memset(g, 0, sizeof(Graphics));
    g->width = w;
    g->height = h;
    g->scale = scale;

    /* Allocate frame buffer */
    int frameWidth = w * scale;
    int frameHeight = h * scale;
    g->frame = malloc(sizeof(uint32_t) * frameWidth * frameHeight);
    if (!g->frame) {
        /* TODO: Implement error checking */
        return;
    }

    /* Make window */
    g->window = SDL_CreateWindow("CHIP8", frameWidth, frameHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!g->window) {
        /* TODO: Implement error checking */
        return;
    }

    /* Make renderer */
    g->renderer = SDL_CreateRenderer(g->window, NULL);
    if (!g->renderer) {
        SDL_DestroyWindow(g->window);
        /* TODO: Implement error checking */
        return;
    }

    /* Make texture */
    g->texture = SDL_CreateTexture(
        g->renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        frameWidth, frameHeight
    );

    if (!g->texture) {
        SDL_DestroyRenderer(g->renderer);
        SDL_DestroyWindow(g->window);
        /* TODO: Implement error checking */
        return;
    }

    /* Initialize window background to solid black */
    SDL_SetRenderDrawColor(g->renderer, 0, 0, 0, 0);
    SDL_RenderClear(g->renderer);
    SDL_RenderPresent(g->renderer);
}

void graphics_shutdown(Graphics *g)
{
    free(g->frame);
    SDL_DestroyTexture(g->texture);
    SDL_DestroyRenderer(g->renderer);
    SDL_DestroyWindow(g->window);
    SDL_Quit();
}

void graphics_copy(Graphics *g, const uint8_t *buffer)
{
    int frameWidth = g->width * g->scale;

    for (uint8_t y = 0; y < g->height; y++) {
        for (uint8_t x = 0; x < g->width; x++) {
            uint8_t pixel = buffer[(y * g->width) + x];

            // Scale the pixel by filling a scaled pixel block
            for (int dy = 0; dy < g->scale; dy++) {
                for (int dx = 0; dx < g->scale; dx++) {
                    // Calculate the corresponding position in the output array
                    int newX = x * g->scale + dx;
                    int newY = y * g->scale + dy;

                    // Set the pixel value in the output (0 or 1)
                    g->frame[(newY * frameWidth) + newX] = (0xFFFFFF00 * pixel) | 0x000000FF;
                }
            }
        }
    }
}

void graphics_draw(const Graphics *g)
{
    SDL_UpdateTexture(g->texture, NULL, g->frame, g->width * g->scale * sizeof(uint32_t));
    SDL_RenderClear(g->renderer);
    SDL_RenderTexture(g->renderer, g->texture, NULL, NULL);
    SDL_RenderPresent(g->renderer);
}
