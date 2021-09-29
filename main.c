#include <stdio.h>
#include <math.h>

#include <tigr.h>


///////////////////////////////////////
// WINDOWS PART
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int Windows_CreateWindow(Tigr **window) {
    // Get the handle of the parent
    // It is a WorkerW window, a previous sibling of Progman
    HWND progman = FindWindow("Progman", NULL);
    if (progman == NULL) return 1;
    // Send the (undocumented) message to trigger the creation of WorkerW in required position
    if (SendMessageTimeout(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, NULL) == 0)
        return 1;
    HWND parent_hwnd = GetWindow(progman, GW_HWNDPREV);
    if (parent_hwnd == NULL) return 1;
    char classname[8];
    if (GetClassName(parent_hwnd, classname, 8) == 0 || strcmp(TEXT("WorkerW"), classname) != 0)
        return 1;

    // Create window; remove titlebar, frame etc; move under the parent
    int w = GetSystemMetrics(SM_CXSCREEN), h = GetSystemMetrics(SM_CYSCREEN);
    *window = tigrWindow(w, h, "Hello", TIGR_AUTO);
    HWND hwnd = (*window)->handle;
    if (SetParent(hwnd, parent_hwnd) == NULL) return 1;
    if (SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_NOACTIVATE) == 0) return 1;
    if (SetWindowLongPtr(hwnd, GWL_STYLE, WS_CHILDWINDOW | WS_VISIBLE) == 0) return 1;
    if (SetWindowPos(hwnd, NULL, 0, 0, w, h,  SWP_NOZORDER | SWP_NOACTIVATE) == 0) return 1;
    return 0;
}
// WINDOWS PART END
//////////////////////////////////////////



int main(int argc, char *argv[]) {
    Tigr *screen;
    Windows_CreateWindow(&screen);
    int i = 0;
    while (!tigrClosed(screen))
    {
        ++i;
        tigrClear(screen, tigrRGB(100 + 50*sin(i*0.1), 0x90, 0xa0));
        tigrPrint(screen, tfont, 120, 110, tigrRGB(0xff, 0xff, 0xff), "Hello, world.");
        tigrUpdate(screen);
    }
    tigrFree(screen);
    return 0;
}
