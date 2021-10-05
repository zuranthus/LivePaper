#pragma once

#include <SDL.h>

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    void *platform_data;
} Context;

void PlatformInit(Context *context);
void PlatformUpdate(Context *context);
void PlatformCleanup(Context *context);