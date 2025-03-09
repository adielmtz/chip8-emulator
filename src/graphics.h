#pragma once

#include <SDL3/SDL.h>
#include <stdint.h>

typedef struct Graphics
{
    uint32_t *frame;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int width;
    int height;
    int scale;
} Graphics;

void graphics_init(Graphics *g, int w, int h, int scale);
void graphics_shutdown(Graphics *g);

void graphics_copy(Graphics *g, const uint8_t *buffer);
void graphics_draw(const Graphics *g);
