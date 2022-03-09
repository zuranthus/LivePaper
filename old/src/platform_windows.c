#include "platform.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <VersionHelpers.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_syswm.h>
#define TRAY_WINAPI 1
#include <tray.h>

#include "fail.h"

extern Uint32 g_open_file_event;

char* OpenVideoFile();

static void TrayQuit(struct tray_menu *item) {
    SDL_QuitEvent event;
    event.type = SDL_QUIT;
    event.timestamp = SDL_GetTicks();
    SDL_PushEvent((SDL_Event*)&event);
}

static void TraySetWallpaper(struct tray_menu *item) {
    char *file = OpenVideoFile();
    if (!file) return;
    
    SDL_Event event;
    SDL_zero(event);
    event.type = g_open_file_event;
    event.user.data1 = file;
    SDL_PushEvent(&event);
}

static struct tray tray = {
    .icon = "icon.ico",
    .menu = (struct tray_menu[]){{.text = "Live Paper", .disabled = true},
                                {.text = "-"},
                                {.text = "Set wallpaper...", .cb = &TraySetWallpaper},
                                {.text = "Quit", .cb = &TrayQuit},
                                {.text = NULL}},
};

static char config_dir[MAX_PATH];
static char config_file[MAX_PATH];

static void InitConfigPath() {
    if (config_file[0]) return; // already initialized
    char appdata[MAX_PATH] = {0};
    HRESULT hr = SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appdata);
    if (FAILED(hr)) FAIL();
    PathCombine(config_dir, appdata, "LivePaper\\");
    PathCombine(config_file, config_dir, "config.txt");
}

static void SaveConfig(const struct Context *context) {
    if (!context->file) return;
    InitConfigPath();
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
    InitConfigPath();
    if (!PathFileExistsA(config_file)) return;
    FILE *f = fopen(config_file, "rt");
    char *file = malloc(MAX_PATH);
    if (fgets(file, MAX_PATH, f) >= 0) {
        assert(!context->file);
        context->file = file;
    }
    fclose(f);
    DeleteFileA(config_file);
}

static char* OpenVideoFile() {
    char *file = malloc(MAX_PATH);
    file[0] = 0;
    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "All Files\0*.*\0\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (!GetOpenFileNameA(&ofn)) {
        free(file);
        file = 0;
    }
    return file;
}

void PlatformInitGuiMode(struct Context *context) {
    if (context->file) FAIL();
    LoadConfig(context);
    if (!context->file)
        context->file = OpenVideoFile();
}

void PlatformInit(struct Context *context) {
    SetProcessDPIAware();
    if (tray_init(&tray) == -1)
        FAIL_WITH("can't create tray with icon '%s'", tray.icon);

    // Get the handle of the parent
    // On Win8+: WorkerW window, a previous sibling of Progman
    // On Win7: Progman window; also need to hide WorkerW window when Aero is on
    HWND progman = FindWindow("Progman", NULL);
    if (progman == NULL) FAIL();
    // Send the (undocumented) message to trigger the creation of WorkerW in required position
    if (SendMessageTimeout(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, NULL) == 0)
        FAIL();
    HWND workerw = GetWindow(progman, GW_HWNDPREV);
    char classname[8] = {0};
    if (workerw && GetClassName(workerw, classname, 8) == 0 || strcmp(TEXT("WorkerW"), classname) != 0)
        workerw = NULL;
    HWND sdl_parent_hwnd = workerw;
    if (!IsWindows8OrGreater()) {
        if (workerw) ShowWindow(workerw, SW_HIDE);
        sdl_parent_hwnd = progman;
    }
    if (!sdl_parent_hwnd) FAIL();

    SDL_Window *window = SDL_CreateWindow("live-paper", 
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 
        SDL_WINDOW_BORDERLESS | SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
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
    ShowWindow(sdl_hwnd, SW_SHOW);

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