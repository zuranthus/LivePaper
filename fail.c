#include "fail.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void MessageAndQuit(int line, const char *message, ...) {
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