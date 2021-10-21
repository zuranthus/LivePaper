#include "fail.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void MessageAndQuit(int line, const char *file, const char *message, ...) {
    va_list args;
    va_start(args, message);

    printf("Error %s:%i", file, line);
    if (message) {
        printf(": ");
        vprintf(message, args);
    }
    puts("\nExiting...");
    fflush(stdout);
    exit(1);
}