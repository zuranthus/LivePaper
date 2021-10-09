#include <assert.h>
#include <math.h>

#include <SDL.h>
#define CUTE_PNG_IMPLEMENTATION
#include <cute_png.h>
#define CUTE_FILES_IMPLEMENTATION
#include <cute_files.h>

#include "fail.h"
#include "platform.h"

typedef struct {
    SDL_Texture **textures;
    int count;
    int w, h;
} Textures;

Textures LoadTextures(const char *dir_path, SDL_Renderer *renderer) {
    Textures textures;
    memset(&textures, 0, sizeof(textures));
    int max_textures = 8;
    textures.textures = malloc(max_textures * sizeof(*textures.textures));

    cf_dir_t dir;
    if (!cf_dir_open(&dir, dir_path))
        FAIL_WITH("can't open directory '%s'", dir_path);
    while (dir.has_next) {
        cf_file_t file;
        cf_read_file(&dir, &file);
        if (file.is_reg && cf_match_ext(&file, ".png")) {
            cp_image_t image = cp_load_png(file.path);
            if (!image.pix) FAIL_WITH("can't load file %s", file.path);
            if (textures.w == 0 || textures.h == 0) {
                textures.w = image.w;
                textures.h = image.h;
            }
            if (image.w != textures.w || image.h != textures.h) {
                FAIL_WITH("previous image(s) are %ix%i pix, but '%s' is %ix%i pix; all images must have identical dimensions", 
                    textures.w, textures.h, file.path, image.w, image.h);
            }
            SDL_Texture *tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, image.w, image.h);
            if (!tex) FAIL_WITH("can't create SDL texture %i x %i pix", image.w, image.h);
            if (SDL_UpdateTexture(tex, NULL, image.pix, image.w*4) != 0) 
                FAIL();

            if (textures.count == max_textures) {
                max_textures *= 2;
                textures.textures = realloc(textures.textures, max_textures * sizeof(*textures.textures));
            }
            textures.textures[textures.count++] = tex;
            free(image.pix);
        }
        cf_dir_next(&dir);
    }
    cf_dir_close(&dir);

    return textures;
}

void ClearTextures(Textures *textures) {
    free(textures->textures);
    memset(textures, 0, sizeof(Textures));
}

Context CreateContext() {
    Context context;
    memset(&context, 0, sizeof(context));
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        FAIL_WITH("can't initialize SDL");
    PlatformInit(&context);
    if (context.window == NULL) FAIL_WITH("can't create window");
    context.renderer = SDL_CreateRenderer(context.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (context.renderer == NULL) FAIL_WITH("can't create renderer");
    return context;
}

void ClearContext(Context *context) {
    if (context->renderer)
        SDL_DestroyRenderer(context->renderer);
    PlatformCleanup(context);
    memset(&context, 0, sizeof(context));
    SDL_Quit();
}

void VideoLoad(char *path, int w, int h);
void VideoNextFrame(SDL_Texture *tex);
void VideoUnload();

int main(int argc, char *argv[]) {
    if (argc < 2) FAIL();
    // if (argc != 3) {
    //     puts("USAGE: animated-wallpaper path-to-dir delay-in-ms");
    //     puts("EXAMPLE: ./animated-wallpaper /home/wallpaper/ 100");
    //     return -1;
    // }

    Context context = CreateContext();
    // Textures textures = LoadTextures(argv[1], context.renderer);
    // if (textures.count == 0) FAIL_WITH("No images loaded from '%s'", argv[1]);
    // const int delay = atoi(argv[2]);

    int win_w, win_h;
    SDL_GetWindowSize(context.window, &win_w, &win_h);
    VideoLoad(argv[1], win_w, win_h);
    SDL_Texture *tex = SDL_CreateTexture(context.renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, win_w, win_h);
        if (!tex) FAIL_WITH("can't create SDL texture %i x %i pix", win_w, win_h);
    // const float aspect_ratio = fmaxf((float)win_w/textures.w, (float)win_h/textures.h);
    // const int target_w = (int)(textures.w * aspect_ratio);
    // const int target_h = (int)(textures.h * aspect_ratio);
    // const SDL_Rect dest_rect = {
    //     .x = (win_w - target_w)/2,
    //     .y = (win_h - target_h)/2,
    //     .w = target_w,
    //     .h = target_h
    // };
    // int curframe = -1;
    for (;;) {
        // int frame = (SDL_GetTicks() / delay) % textures.count;
        // if (curframe != frame) {
        //     curframe = frame;
        //     assert(curframe < textures.count);
            SDL_SetRenderDrawColor(context.renderer, 0, 0, 0, 255);
            SDL_RenderClear(context.renderer);
        //     SDL_RenderCopy(context.renderer, textures.textures[curframe], NULL, &dest_rect);
        SDL_RenderCopy(context.renderer, tex, NULL, NULL);
            SDL_RenderPresent(context.renderer);
        // }
        VideoNextFrame(tex);
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

    SDL_DestroyTexture(tex);
   // ClearTextures(&textures);
    ClearContext(&context);
    VideoUnload();
    return 0;
}