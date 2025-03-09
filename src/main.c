#include <SDL3/SDL.h>

int main(void)
{
    if (!SDL_Init(SDL_INIT_VIDEO /*| SDL_INIT_AUDIO*/ | SDL_INIT_EVENTS)) {
        return; /* TODO: Return an ERROR here */
    }

    SDL_Window *window = SDL_CreateWindow("CHIP-8", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        return; /* TODO: Return an ERROR here */
    }

    SDL_Delay(5000);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
