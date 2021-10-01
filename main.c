#include <SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

struct Context {
    SDL_Window *window;
    SDL_Renderer *renderer;
};

///////////////////////////////////////
// WINDOWS PART
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <SDL_syswm.h>

int Windows_CreateWindow(SDL_Window **out_window) {
    // Get the handle of the parent
    // It is a WorkerW window, a previous sibling of Progman
    HWND progman = FindWindow("Progman", NULL);
    if (progman == NULL) return 1;
    // Send the (undocumented) message to trigger the creation of WorkerW in required position
    if (SendMessageTimeout(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, NULL) == 0)
        return 1;
    HWND sdl_parent_hwnd = GetWindow(progman, GW_HWNDPREV);
    if (sdl_parent_hwnd == NULL) return 1;
    char classname[8];
    if (GetClassName(sdl_parent_hwnd, classname, 8) == 0 || strcmp(TEXT("WorkerW"), classname) != 0)
        return 1;

    SDL_Window *window = SDL_CreateWindow("animated-wallpaper", 
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 
        SDL_WINDOW_BORDERLESS | SDL_WINDOW_OPENGL);
    if (window == NULL) return 1;
    *out_window = window;

    // Attach SDL window to the parent
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (!SDL_GetWindowWMInfo(window, &info)) return 1;
    HWND sdl_hwnd = info.info.win.window;
    assert(sdl_hwnd != NULL);
    if (SetParent(sdl_hwnd, sdl_parent_hwnd) == NULL) return 1;
    if (SetWindowLong(sdl_hwnd, GWL_EXSTYLE, WS_EX_NOACTIVATE) == 0) return 1;

    return 0;
}

// WINDOWS PART END
////////////////////////////////////////



int Init(struct Context *context) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
    if (Windows_CreateWindow(&context->window) != 0) return 1;
    context->renderer = SDL_CreateRenderer(context->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (context->renderer == NULL) return 1;

    return 0;
}

void Shutdown(struct Context *context) {
    if (context->renderer)
        SDL_DestroyRenderer(context->renderer);
    if (context->window)
        SDL_DestroyWindow(context->window);

    SDL_Quit();
}

int main(int argc, char *argv[]) {
    struct Context context = {0};

    int result = 0;
    if (Init(&context) != 0) {
        result = 1;
        goto cleanup;
    }

    int i = 0;
    for (;;) {
        // test rendering
		SDL_SetRenderDrawColor(context.renderer, (uint8_t)(50 + 25*(1 + sin(i*0.1))), 0, 0, 255);
		SDL_RenderClear(context.renderer);
		SDL_RenderPresent(context.renderer);
		i++;

		SDL_Delay(50);

        SDL_Event event;
        SDL_PollEvent(&event);
        if(event.type == SDL_QUIT) {
            break;
        }
    }

cleanup:
    Shutdown(&context);
    return result;
}