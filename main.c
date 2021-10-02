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
} Textures;

Textures LoadTextures(const char *dir_path, SDL_Renderer *renderer) {
    int max_textures = 8;
    Textures textures;
    textures.count = 0;
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
    const int delay = atoi(argv[2]);
    int curframe = -1;
    for (;;) {
        int frame = (SDL_GetTicks() / delay) % textures.count;
        if (curframe != frame) {
            curframe = frame;
            assert(curframe < textures.count);
            SDL_SetRenderDrawColor(context.renderer, 0, 0, 0, 255);
            SDL_RenderClear(context.renderer);
            SDL_RenderCopy(context.renderer, textures.textures[curframe], NULL, NULL);
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