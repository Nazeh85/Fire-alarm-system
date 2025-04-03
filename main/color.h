#include "stdio.h"

#ifndef COLOR_H
#define COLOR_H

static const char *black = "\033[0;30m";
static const char *red = "\033[0;31m";
static const char *green = "\033[0;32m";
static const char *yellow = "\033[0;33m";
static const char *blue = "\033[0;34m";
static const char *magenta = "\033[0;35m";
static const char *cyan = "\033[0;36m";
static const char *white = "\033[0;37m";
static const char *reset = "\033[0m";

#define PRINTFC(color, format, ...) printf("%s" format "%s", color, ##__VA_ARGS__, reset)

#define PRINTFC_MAIN(format, ...)  \
    PRINTFC(yellow, "Main: ");     \
    printf(format, ##__VA_ARGS__); \
    printf("\n")




#endif