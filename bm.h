#pragma once
#include <assert.h>

#include <stddef.h>
#include <stdbool.h>

// Bitmask operations
// Uses uint64_t arrays as underlying store

#define BM_SET(BS, I) bm_set((BS), sizeof(BS)/sizeof(uint64_t), (I))
static inline void
bm_set(uint64_t *bs, size_t n, size_t i) {
    assert(i < (n * 64));
    bs[i / 64] |= 1ul << (i % 64);
}

/*
#define BM_CLEAR(BS, I) bm_clear((BS), sizeof(BS)/sizeof(uint64_t), (I))
static inline void
bm_clear(uint64_t *bs, size_t n, size_t i) {
    assert(i < (n * 64));
    bs[i / 64] &= ~(1ul << (i % 64));
}
*/

#define BM_ISSET(BS, I) bm_isset((BS), sizeof(BS)/sizeof(uint64_t), (I))
static inline bool
bm_isset(const uint64_t *bs, size_t n, size_t i) {
    assert(i < (n * 64));
    return !!(bs[i / 64] & (1ul << (i % 64)));
}

#define BM_ISEMPTY(BS) bm_isempty((BS), sizeof(BS)/sizeof(uint64_t))
static inline bool
bm_isempty(const uint64_t *bs, size_t n) {
    for(; n; n--) {
        if (*bs++) return false;
    }
    return true;
}

#define BM_ISMATCH(BS, INVS) bm_ismatch((BS), (INVS), sizeof(BS)/sizeof(uint64_t))
static inline bool
bm_ismatch(const uint64_t *bs, const uint64_t *invs, size_t n) {
    // This is an operation needed by ap_req.c,
    // if `invs` satisfies the requirements in `bs`
    for(; n; n--) {
        if (*bs++ & ~*invs++) return false;
    }
    return true;
}

#define BM_ANDEQ(BS, XS) bm_andeq((BS), (XS), sizeof(BS)/sizeof(uint64_t))
static inline void
bm_andeq(uint64_t *bs, const uint64_t *xs, size_t n) {
    for(; n; n--) {
        *bs++ &= *xs++;
    }
}

#define BM_POPCOUNT(BS) bm_popcount((BS), sizeof(BS)/sizeof(uint64_t))
static inline size_t
bm_popcount(const uint64_t *bs, size_t n) {
    size_t c = 0;
    static_assert(sizeof(unsigned long) == sizeof(uint64_t), "Update __builtin_popcountl to match uint64_t");
    for(; n; n--) {
        c += __builtin_popcountl(*bs++);
    }
    return c;
}

#define BM_FFZ(BS) bm_ffz((BS), sizeof(BS)/sizeof(uint64_t))
static inline size_t 
bm_ffz(const uint64_t *bs, size_t n) {
    static_assert(sizeof(unsigned long) == sizeof(uint64_t), "Update __builtin_ffsl to match uint64_t");
    for (size_t i = 0; i < n; i++) {
        size_t z = __builtin_ffsl(~bs[i]);
        if (z != 0) {
            return (z - 1) + (i * 64);
        }
    }
    return n * 64;
}
