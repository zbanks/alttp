#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONCAT(x, y) CONCAT2(x, y)
#define CONCAT2(x, y) x ## y

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) # x

#define ARRAYLEN(x) (sizeof(x) / sizeof(*x))

#define TERM_BOLD(x)    "\033[1m"  x "\033[0m"
#define TERM_RED(x)     "\x1b[31m" x "\x1b[0m"
#define TERM_GREEN(x)   "\x1b[32m" x "\x1b[0m"
#define TERM_YELLOW(x)  "\x1b[33m" x "\x1b[0m"
#define TERM_BLUE(x)    "\x1b[34m" x "\x1b[0m"
#define TERM_MAGENTA(x) "\x1b[35m" x "\x1b[0m"
#define TERM_CYAN(x)    "\x1b[36m" x "\x1b[0m"

#define LOG(fmt, ...) printf("[%-6lu "__FILE__ ":%s:%d] " fmt "\n", ap_frame, __func__, __LINE__, ## __VA_ARGS__)
#define LOGB(fmt, ...) LOG(TERM_BOLD(fmt), ## __VA_ARGS__)
#define DEBUG(...) ({ if(ap_debug){LOG(__VA_ARGS__);} })
#define INFO_STRING_SIZE 256
#define INFO(...) snprintf(ap_info_string, INFO_STRING_SIZE, __VA_ARGS__)
#define INFO_HEXDUMP(ptr) INFO_HEXDUMP2(((uint8_t *)ptr))
#define INFO_HEXDUMP2(p) INFO("%02X %02X %02X %02X.%02X %02X %02X %02X.%02X %02X %02X %02X", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11]); 

extern char ap_info_string[];
extern bool ap_debug;
extern uint64_t ap_frame;

#define LL_INIT(list) ({ \
    (list)->prev = (list); \
    (list)->next = (list); \
    (list); })

#define LL_PUSH(list, node) ({ \
    assert((node)->next == (node) && (node)->prev == (node)); \
    (node)->prev = (list)->prev; \
    (node)->prev->next = (node); \
    (node)->next = (list); \
    (node)->next->prev = (node); })

#define LL_PREPEND(list, node) ({ \
    assert((node)->next == (node) && (node)->prev == (node)); \
    (node)->next = (list)->next; \
    (node)->next->prev = (node); \
    (node)->prev = (list); \
    (node)->prev->next = (node); })

#define LL_POP(list) ({ \
    __auto_type n = (list)->next; \
    (list)->next = (list)->next->next; \
    (list)->next->prev = (list); \
    (n == (list)) ? NULL : LL_INIT(n); })

#define LL_EXTRACT(node) ({ \
    (node)->prev->next = (node)->next; \
    (node)->next->prev = (node)->prev; \
    LL_INIT(node); \
    })

#define LL_PEEK(list) ({ \
    (list)->next == (list) ? NULL : (list)->next; })

enum rc {
    RC_FAIL = -1,
    RC_DONE = 0,
    RC_INPR = 1,
};

#define NONNULL(x) ({ __typeof__(x) _x = (x); assert((_x) != NULL); _x; })
#define assert_bp(x) ({ if (!(x)) { __asm__("int3"); volatile bool _marker = 0; } })
//#define assert_bp(x) ((void) 0)
