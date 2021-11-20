#include "platform.h"

#include <SDL.h>
#include <X11/Xlib.h>

#include "fail.h"

void PlatformInitGuiMode(struct Context *context) {}

void PlatformInit(struct Context *context) {
    Display *x_display = XOpenDisplay(NULL);
    if (x_display == NULL) FAIL_WITH("can't open X11 display");
    Window x_window = XDefaultRootWindow(x_display);
    if (!x_window) FAIL();
    context->window = SDL_CreateWindowFrom((void*)x_window);
    if (context->window == NULL) FAIL_WITH("can't create SDL window");
}

void PlatformUpdate(struct Context *context) {}

void PlatformCleanup(struct Context *context) {
    if (context->window)
        SDL_DestroyWindow(context->window);
    context->window = NULL;
    Display *x_display = context->platform_data;
    if (x_display) XCloseDisplay(x_display);
    context->platform_data = NULL;
}