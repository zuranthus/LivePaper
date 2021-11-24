#include "fail.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

void MessageAndQuit(int line, const char *file, const char *message, ...) {
    va_list args;
    va_start(args, message);

    char buf[4096] = {0};
    int pos = 0;
    pos += sprintf(buf + pos, "Error %s:%i", file, line);
    if (message) {
        pos += sprintf(buf + pos, ": ");
        pos += vsprintf(buf + pos, message, args);
    }
    va_end(args);
#ifdef _WIN32
    MessageBoxA(NULL, buf, "LivePaper Error", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
    puts(buf);
    puts("Exiting...");
    fflush(stdout);
#endif
    
    exit(1);
}