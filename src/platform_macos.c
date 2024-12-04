#include "platform.h"

#include <SDL.h>

#include "fail.h"

void PlatformInitGuiMode(struct Context *context) {}

void PlatformInit(struct Context *context)
{
    context->window = SDL_CreateWindow("LivePaper TEST", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1000, 500, SDL_WINDOW_SHOWN);
    if (context->window == NULL)
        FAIL_WITH("can't create SDL window");
}

void PlatformUpdate(struct Context *context) {}

void PlatformCleanup(struct Context *context)
{
    if (context->window)
        SDL_DestroyWindow(context->window);
    context->window = NULL;
}