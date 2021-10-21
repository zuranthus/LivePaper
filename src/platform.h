#pragma once

#include <SDL.h>

struct Context {
    SDL_Window *window;
    SDL_Renderer *renderer;
    void *platform_data;
};

void PlatformInit(struct Context *context);
void PlatformUpdate(struct Context *context);
void PlatformCleanup(struct Context *context);
