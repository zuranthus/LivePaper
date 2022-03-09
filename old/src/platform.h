#pragma once

#include <stdbool.h>
#include <SDL.h>

enum {
    FIT_FIT,
    FIT_FILL,
    FIT_CENTER
};

struct Context {
    SDL_Window *window;
    SDL_Renderer *renderer;
    void *platform_data;

    char *file;
    int fit;
    bool cache;
};

void PlatformInitGuiMode(struct Context *context);
void PlatformInit(struct Context *context);
void PlatformUpdate(struct Context *context);
void PlatformCleanup(struct Context *context);
