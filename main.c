#include <assert.h>
#include <SDL.h>
#define CUTE_PNG_IMPLEMENTATION
#include <cute_png.h>
#define CUTE_FILES_IMPLEMENTATION
#include <cute_files.h>

#undef ERROR
#define ERROR_M(M, ...) error_impl(__LINE__, M, __VA_ARGS__)
#define ERROR() error_impl(__LINE__, NULL)

static void error_impl(int line, const char *message, ...) {
    va_list args;
    va_start(args, message);

    printf("Error line %i", line);
    if (message) {
        printf(": ");
        vprintf(message, args);
    }
    puts("\nExiting...");
    fflush(stdout);
    exit(1);
}

///////////////////////////////////////
// WINDOWS PART
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <SDL_syswm.h>
#undef LoadImage

SDL_Window* Windows_CreateWindow() {
    // Get the handle of the parent
    // It is a WorkerW window, a previous sibling of Progman
    HWND progman = FindWindow("Progman", NULL);
    if (progman == NULL) ERROR();
    // Send the (undocumented) message to trigger the creation of WorkerW in required position
    if (SendMessageTimeout(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, NULL) == 0)
        ERROR();
    HWND sdl_parent_hwnd = GetWindow(progman, GW_HWNDPREV);
    if (sdl_parent_hwnd == NULL) ERROR();
    char classname[8];
    if (GetClassName(sdl_parent_hwnd, classname, 8) == 0 || strcmp(TEXT("WorkerW"), classname) != 0)
        ERROR();

    SDL_Window *window = SDL_CreateWindow("animated-wallpaper", 
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 
        SDL_WINDOW_BORDERLESS | SDL_WINDOW_OPENGL);
    if (window == NULL) ERROR();

    // Attach SDL window to the parent
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (!SDL_GetWindowWMInfo(window, &info)) 
        ERROR();
    HWND sdl_hwnd = info.info.win.window;
    assert(sdl_hwnd != NULL);
    if (SetParent(sdl_hwnd, sdl_parent_hwnd) == NULL) 
        ERROR();
    if (SetWindowLong(sdl_hwnd, GWL_EXSTYLE, WS_EX_NOACTIVATE) == 0) 
        ERROR();

    return window;
}

// WINDOWS PART END
////////////////////////////////////////

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
} Context;

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
        ERROR_M("can't open directory '%s'", dir_path);
    while (dir.has_next) {
        cf_file_t file;
        cf_read_file(&dir, &file);
        if (file.is_reg && cf_match_ext(&file, ".png")) {
            cp_image_t image = cp_load_png(file.path);
            if (!image.pix) ERROR_M("can't load file %s", file.path);
            if (textures.w == 0 || textures.h == 0) {
                textures.w = image.w;
                textures.h = image.h;
            }
            if (image.w != textures.w || image.h != textures.h) {
                ERROR_M("previous image(s) are %ix%i pix, but '%s' is %ix%i pix; all images must have identical dimensions", 
                    textures.w, textures.h, file.path, image.w, image.h);
            }
            SDL_Texture *tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, image.w, image.h);
            if (!tex) ERROR_M("can't create SDL texture %i x %i pix", image.w, image.h);
            if (SDL_UpdateTexture(tex, NULL, image.pix, image.w*4) != 0) 
                ERROR();

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
        ERROR_M("can't initialize SDL");
    context.window = Windows_CreateWindow(&context.window);
    if (context.window == NULL) ERROR_M("can't create window");
    context.renderer = SDL_CreateRenderer(context.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (context.renderer == NULL) ERROR_M("can't create renderer");
    return context;
}

void ClearContext(Context *context) {
    if (context->renderer)
        SDL_DestroyRenderer(context->renderer);
    if (context->window)
        SDL_DestroyWindow(context->window);
    memset(&context, 0, sizeof(context));
    SDL_Quit();
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        puts("USAGE: animated-wallpaper path-to-dir delay-in-ms");
        puts("EXAMPLE: ./animated-wallpaper /home/wallpaper/ 100");
        return -1;
    }

    Context context = CreateContext();
    Textures textures = LoadTextures(argv[1], context.renderer);
    if (textures.count == 0) ERROR_M("No images loaded from '%s'", argv[1]);
    const int delay = atoi(argv[2]);

    int win_w, win_h;
    SDL_GetWindowSize(context.window, &win_w, &win_h);
    const float aspect_ratio = max((float)win_w/textures.w, (float)win_h/textures.h);
    const int target_w = (int)(textures.w * aspect_ratio);
    const int target_h = (int)(textures.h * aspect_ratio);
    const SDL_Rect dest_rect = {
        .x = (win_w - target_w)/2,
        .y = (win_h - target_h)/2,
        .w = target_w,
        .h = target_h
    };
    int curframe = -1;
    for (;;) {
        int frame = (SDL_GetTicks() / delay) % textures.count;
        if (curframe != frame) {
            curframe = frame;
            assert(curframe < textures.count);
            SDL_SetRenderDrawColor(context.renderer, 0, 0, 0, 255);
            SDL_RenderClear(context.renderer);
            SDL_RenderCopy(context.renderer, textures.textures[curframe], NULL, &dest_rect);
            SDL_RenderPresent(context.renderer);
        }

		SDL_Delay(10);

        SDL_Event event;
        SDL_PollEvent(&event);
        if(event.type == SDL_QUIT) {
            break;
        }
    }

    ClearTextures(&textures);
    ClearContext(&context);
    return 0;
}