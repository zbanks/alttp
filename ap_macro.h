#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CONCAT(x, y) CONCAT2(x, y)
#define CONCAT2(x, y) x ## y

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) # x

#define TERM_BOLD(x)    "\033[1m"  x "\033[0m"
#define TERM_RED(x)     "\x1b[31m" x "\x1b[0m"
#define TERM_GREEN(x)   "\x1b[32m" x "\x1b[0m"
#define TERM_YELLOW(x)  "\x1b[33m" x "\x1b[0m"
#define TERM_BLUE(x)    "\x1b[34m" x "\x1b[0m"
#define TERM_MAGENTA(x) "\x1b[35m" x "\x1b[0m"
#define TERM_CYAN(x)    "\x1b[36m" x "\x1b[0m"

#define DEBUG(fmt, ...) ({ if(ap_debug){printf(fmt "\n", ## __VA_ARGS__);} })
#define INFO_STRING_SIZE 256
#define INFO(...) snprintf(ap_info_string, INFO_STRING_SIZE, __VA_ARGS__)
#define INFO_HEXDUMP(ptr) INFO_HEXDUMP2(((uint8_t *)ptr))
#define INFO_HEXDUMP2(p) INFO("%02X %02X %02X %02X.%02X %02X %02X %02X.%02X %02X %02X %02X", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11]); 

extern char ap_info_string[];
extern bool ap_debug;

