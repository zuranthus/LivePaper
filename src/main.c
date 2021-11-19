#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <SDL.h>
#include <argtable3.h>

#include "fail.h"
#include "platform.h"
#include "video.h"

extern void PlatformLoadConfig(struct Context *context);
extern void PlatformPreinit();

void InitContext(struct Context *ctx) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) FAIL_WITH("can't initialize SDL");
    PlatformInit(ctx);
    if (ctx->window == NULL) FAIL_WITH("can't create window");
    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ctx->renderer == NULL) FAIL_WITH("can't create renderer");
}

void ClearContext(struct Context *context) {
    if (context->renderer) SDL_DestroyRenderer(context->renderer);
    PlatformCleanup(context);
    SDL_Quit();
    if (context->file) free(context->file);
    memset(&context, 0, sizeof(context));
}

void ProcessArguments(int argc, char *argv[], struct Context *context) {
    const char progname[] = "live-paper";
    struct arg_lit *help;
    struct arg_str  *fit;
    struct arg_lit *cache;
    struct arg_file *file;
    struct arg_end *end;
    void *argtable[] = {
        help = arg_lit0("h", "help", "= display this help and exit"),
        fit = arg_str0(NULL, "fit-mode", "<mode>", ""),
            arg_rem(NULL, "= controls the way the wallpaper is fit on screen"),
            arg_rem(NULL, "  possible values: fit, fill, center"),
        cache = arg_lit0(NULL, "cache", "= decode all frames at once and store them in memory"),
            arg_rem(NULL, "  this option is available for short clips only (<=16 frames)"),
        file = arg_file0(NULL, NULL, "<file>", "= video or animation file to display"),
        end = arg_end(20)
    };
    if (arg_nullcheck(argtable) != 0) FAIL();
    int nerrors = arg_parse(argc, argv, argtable);
    if (help->count > 0) {
        printf("Usage: %s", progname);
        arg_print_syntax(stdout, argtable, "\n");
        printf("Display a video or an animated file as desktop background.\n\n");
        arg_print_glossary(stdout, argtable, "  %-12s %s\n");
        printf("\nExamples: %s loop.mp4\n"
                 "          %s --fit-mode=fill --cache wallpaper.gif\n",
            progname, progname);
        exit(0);
    }
    if (nerrors > 0) {
        arg_print_errors(stdout, end, progname);
        printf("Try '%s --help' for more information.", progname);
        exit(1);
    }
    
    if (file->count > 0) {
        context->file = strdup(file->filename[0]);
    } else {
        PlatformLoadConfig(context);
    }
    if (!context->file) FAIL();
    context->cache = (cache->count > 0);
    context->fit = FIT_FIT;
    if (fit->count) {
        if (strcmp(fit->sval[0], "fill") == 0) context->fit = FIT_FILL;
        else if (strcmp(fit->sval[0], "center") == 0) context->fit = FIT_CENTER;
    }
    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
}

int main(int argc, char *argv[]) {
    PlatformPreinit();
    struct Context context = {0};
    ProcessArguments(argc, argv, &context);
    InitContext(&context);
    struct Video* video = VideoLoad(&context);
    
    double ticks_frequency = (double)SDL_GetPerformanceFrequency();
    uint64_t ticks_now = SDL_GetPerformanceCounter();
    uint64_t ticks_last = 0;
    for (;;) {
        ticks_last = ticks_now;
        ticks_now = SDL_GetPerformanceCounter();
        double delta_sec = (ticks_now - ticks_last)/ticks_frequency;

        VideoUpdate(delta_sec, video, &context);
        PlatformUpdate(&context);

        SDL_Event event;
        SDL_PollEvent(&event);
        if(event.type == SDL_QUIT) break;
        SDL_Delay(1);
    }

    VideoClear(video, &context);
    ClearContext(&context);
    return 0;
}