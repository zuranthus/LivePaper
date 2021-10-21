#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <SDL.h>

#include "fail.h"
#include "platform.h"
#include "video.h"

struct Context CreateContext() {
    struct Context context;
    memset(&context, 0, sizeof(context));
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        FAIL_WITH("can't initialize SDL");
    PlatformInit(&context);
    if (context.window == NULL) FAIL_WITH("can't create window");
    context.renderer = SDL_CreateRenderer(context.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (context.renderer == NULL) FAIL_WITH("can't create renderer");
    return context;
}

void ClearContext(struct Context *context) {
    if (context->renderer)
        SDL_DestroyRenderer(context->renderer);
    PlatformCleanup(context);
    memset(&context, 0, sizeof(context));
    SDL_Quit();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        puts("USAGE: animated-wallpaper path-to-file");
        puts("EXAMPLES: ./animated-wallpaper ~/wallpaper/fuji.gif");
        puts("          ./animated-wallpaper ~/wallpaper/video.mp4");
        return -1;
    }

    struct Context context = CreateContext();
    struct Video* video = VideoLoad(argv[1], &context);
    
    double ticks_frequency = (double)SDL_GetPerformanceFrequency();
    uint64_t ticks_now = SDL_GetPerformanceCounter();
    uint64_t ticks_last = 0;
    for (;;) {
        ticks_last = ticks_now;
        ticks_now = SDL_GetPerformanceCounter();
        double delta_sec = (ticks_now - ticks_last)/ticks_frequency;

        VideoUpdate(delta_sec, video, &context);
        
        fflush(stdout);
        PlatformUpdate(&context);
        SDL_Event event;
        SDL_PollEvent(&event);
        if(event.type == SDL_QUIT) {
            SDL_SetRenderDrawColor(context.renderer, 0, 0, 0, 255);
            SDL_RenderClear(context.renderer);
            SDL_RenderPresent(context.renderer);
            break;
        }
        SDL_Delay(1);
    }

    VideoClear(video, &context);
    ClearContext(&context);
    return 0;
}