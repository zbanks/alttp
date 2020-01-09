#ifndef __LB_H__
#define __LB_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Logic Bitmask
// Data structure for solving bipartite perfect matching

#define LB_SIZE ((size_t) 256)
#define LB_BM_SIZE (LB_SIZE / 64)
struct lb {
    struct lb_half {
        uint64_t bs[LB_SIZE][LB_BM_SIZE];
        uint64_t paired[LB_BM_SIZE];
        uint64_t pair[LB_SIZE];
    } halfs[2];
    uint16_t x_root[LB_SIZE];
    bool x_has_equivs[LB_SIZE];
    uint64_t x_equivs[LB_SIZE][LB_BM_SIZE];

    size_t size;
    bool dirty;
    bool marked;
};

void lb_init(struct lb * lb, size_t n);
void lb_init_equivalent(struct lb * lb, size_t x0, size_t x1);

void lb_mark_positive(struct lb * lb, size_t x, size_t y);
void lb_mark_negative(struct lb * lb, size_t x, size_t y);

void lb_deduce(struct lb * lb);

#ifdef LB_DEBUG 
void lb_selftest(void);
#endif

#endif
