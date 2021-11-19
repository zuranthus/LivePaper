#include "platform.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_syswm.h>
#define TRAY_WINAPI 1
#include <tray.h>

#include "fail.h"

void TrayQuit(struct tray_menu *item) {
    SDL_QuitEvent event;
    event.type = SDL_QUIT;
    event.timestamp = SDL_GetTicks();
    SDL_PushEvent((SDL_Event*)&event);
}

static struct tray tray = {
    .icon = "icon.ico",
    .menu = (struct tray_menu[]){{.text = "Live Paper", .disabled = true},
                                {.text = "-"},
                                {.text = "Quit", .cb = &TrayQuit},
                                {.text = NULL}},
};

static char config_dir[MAX_PATH];
static char config_file[MAX_PATH];

static void SaveConfig(const struct Context *context) {
    if (!context->file) return;
    char file[MAX_PATH] = {0};
    if (PathIsRelativeA(context->file)) {
        _fullpath(file, context->file, MAX_PATH);
    } else {
        strcpy(file, context->file);
    }

    SHCreateDirectoryExA(NULL, config_dir, NULL);
    FILE *f = fopen(config_file, "wt");
    fputs(file, f);
    fclose(f);
}

static void LoadConfig(struct Context *context) {
    if (!PathFileExistsA(config_file)) return;
    FILE *f = fopen(config_file, "rt");
    char *file = malloc(MAX_PATH);
    if (fgets(file, MAX_PATH, f) >= 0) {
        assert(!context->file);
        context->file = file;
    }
    fclose(f);
}

void PlatformLoadConfig(struct Context *context) {
    if (!context->file) {
        LoadConfig(context);
    }
}

void PlatformPreinit() {
    char appdata[MAX_PATH] = {0};
    HRESULT hr = SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appdata);
    if (FAILED(hr)) FAIL();
    PathCombine(config_dir, appdata, "LivePaper\\");
    PathCombine(config_file, config_dir, "config.txt");
}

void PlatformInit(struct Context *context) {
    SetProcessDPIAware();
    if (tray_init(&tray) == -1)
        FAIL_WITH("can't create tray with icon '%s'", tray.icon);

    // Get the handle of the parent
    // It is a WorkerW window, a previous sibling of Progman
    HWND progman = FindWindow("Progman", NULL);
    if (progman == NULL) FAIL();
    // Send the (undocumented) message to trigger the creation of WorkerW in required position
    if (SendMessageTimeout(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, NULL) == 0)
        FAIL();
    HWND sdl_parent_hwnd = GetWindow(progman, GW_HWNDPREV);
    if (sdl_parent_hwnd == NULL) FAIL();
    char classname[8];
    if (GetClassName(sdl_parent_hwnd, classname, 8) == 0 || strcmp(TEXT("WorkerW"), classname) != 0)
        FAIL();

    SDL_Window *window = SDL_CreateWindow("live-paper", 
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 
        SDL_WINDOW_BORDERLESS | SDL_WINDOW_OPENGL);
    if (window == NULL) FAIL();

    // Attach SDL window to the parent
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (!SDL_GetWindowWMInfo(window, &info)) 
        FAIL();
    HWND sdl_hwnd = info.info.win.window;
    assert(sdl_hwnd != NULL);
    if (SetParent(sdl_hwnd, sdl_parent_hwnd) == NULL) 
        FAIL();
    if (SetWindowLong(sdl_hwnd, GWL_EXSTYLE, WS_EX_NOACTIVATE) == 0) 
        FAIL();

    context->window = window;
}

void PlatformUpdate(struct Context *context) {
    tray_loop(0);
}

void PlatformCleanup(struct Context *context) {
    SaveConfig(context);
    if (context->window)
        SDL_DestroyWindow(context->window);
    context->window = NULL;
    tray_exit();
}