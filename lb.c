#include "lb.h"
#include "bm.h"
#include <assert.h>
#include <string.h>
#ifdef LB_DEBUG
#include <stdio.h>
#endif

void
lb_init(struct lb * lb, size_t n) {
    assert(n <= LB_SIZE);
    assert(n > 0);
    memset(lb, 0, sizeof(*lb));
    lb->size = n;
    lb->dirty = true;
    for (size_t x = 0; x < lb->size; x++) {
        lb->x_root[x] = x;
        lb->x_has_equivs[x] = false;
        BM_SET(lb->x_equivs[x], x);
    }
}

void
lb_init_equivalent(struct lb * lb, size_t x0, size_t x1) {
    assert(x0 < x1);
    assert(x1 < lb->size);
    assert(lb->x_root[x0] == x0);
    assert(!lb->x_has_equivs[x1]);
    assert(!lb->marked);
    if (lb->x_root[x1] == x0) return;
    assert(lb->x_root[x1] == x1);

    lb->x_root[x1] = x0;
    BM_SET(lb->x_equivs[x0], x1);
    lb->x_has_equivs[x0] = true;
}

static void
lb_mark_positive_half(struct lb * lb, bool h, size_t x, size_t y) {
    assert(lb->halfs[h].pair[x] == 0);
    for (size_t i = 0; i < lb->size; i++) {
        if (i != x) {
            BM_SET(lb->halfs[h].bs[i], y);
        }
        if (i != y) {
            BM_SET(lb->halfs[h].bs[x], i);
        }
    }
    BM_SET(lb->halfs[h].paired, x);
    lb->halfs[h].pair[x] = y;
    lb->dirty = true;
}

void
lb_mark_positive(struct lb * lb, size_t x, size_t y) {
    assert(x < lb->size);
    assert(y < lb->size);
    assert(!BM_ISSET(lb->halfs[0].bs[x], y));
    assert(!BM_ISSET(lb->halfs[1].bs[y], x));
    if (BM_ISSET(lb->halfs[0].paired, x)) {
        assert(BM_ISSET(lb->halfs[1].paired, y));
        assert(lb->halfs[0].pair[x] == y);
        assert(lb->halfs[1].pair[y] == x);
        return;
    }
    assert(!BM_ISSET(lb->halfs[1].paired, y));

    for (size_t i = lb->x_root[x]; i < lb->size; i++) {
        if (!BM_ISSET(lb->x_equivs[x], i)) {
            continue;
        }
        if (BM_ISSET(lb->halfs[0].paired, i)) {
            continue;
        }
        lb_mark_positive_half(lb, 0, i, y);
        lb_mark_positive_half(lb, 1, y, i);
        return;
    }
    assert(0);
}

static void
lb_mark_negative_half(struct lb * lb, bool h, size_t x, size_t y) {
    if (BM_ISSET(lb->halfs[h].paired, x)) {
        assert(lb->halfs[h].pair[x] != y);
    }
    if (!BM_ISSET(lb->halfs[h].bs[x], y)) {
        BM_SET(lb->halfs[h].bs[x], y);
        lb->dirty = true;
    }
}

void
lb_mark_negative(struct lb * lb, size_t x, size_t y) {
    assert(x < lb->size);
    assert(y < lb->size);
    for (size_t i = lb->x_root[x]; i < lb->size; i++) {
        if (!BM_ISSET(lb->x_equivs[x], i)) {
            continue;
        }
        lb_mark_negative_half(lb, 0, i, y);
        lb_mark_negative_half(lb, 1, y, i);
        if (!lb->x_has_equivs[x]) {
            break;
        }
    }
}

static void
lb_deduce_unique(struct lb * lb, bool h) {
    // Look for cases where there is only 1 possibility left
    for (size_t i = 0; i < lb->size; i++) {
        if (BM_ISSET(lb->halfs[h].paired, i)) {
            continue;
        }
        size_t p = BM_POPCOUNT(lb->halfs[h].bs[i]);
        assert(p < lb->size);
        if (p == lb->size - 1) {
            size_t z = BM_FFZ(lb->halfs[h].bs[i]);
            assert(z < lb->size);
            lb_mark_positive_half(lb, h, i, z);
            lb_mark_positive_half(lb, !h, z, i);
        }
    }
}

static void
lb_deduce_clique(struct lb * lb, bool h) {
    // Look for self-contained subsets
    size_t l = 0;
    for (size_t i = 0; i < lb->size; i++) {
        if (BM_ISSET(lb->halfs[h].paired, i)) {
            continue;
        }
        size_t p = lb->size - BM_POPCOUNT(lb->halfs[h].bs[i]);
        if (p <= lb->size / 2) {
            uint64_t js[LB_BM_SIZE];
            memset(js, 0xFF, sizeof(js));
            for (size_t j = 0; j < lb->size; j++) {
                if (!BM_ISSET(lb->halfs[h].bs[i], j)) {
                    continue;
                }
                BM_ANDEQ(js, lb->halfs[!h].bs[j]);
            }
            size_t q = BM_POPCOUNT(js);
            assert(q <= p); // Hill's Criteria
            if (q == p) {
                // Found a clique
                for (size_t x = 0; x < lb->size; x++) {
                    if (BM_ISSET(js, x)) {
                        continue;
                    }
                    for (size_t y = 0; y < lb->size; y++) {
                        if (BM_ISSET(lb->halfs[h].bs[i], y)) {
                            continue;
                        }
                        lb_mark_negative_half(lb, h, x, y);
                        lb_mark_negative_half(lb, !h, y, x);
                    }
                }
                return;
            }
        }
    }
}

static void
lb_deduce_equivalents(struct lb * lb) {
    for (size_t x = 0; x < lb->size; x++) {
        if (!lb->x_has_equivs[x]) {
            continue;
        }

        uint64_t bs_union[LB_BM_SIZE];
        size_t union_count = 0;
        for (size_t i = lb->x_root[x]; i < lb->size; i++) {
            if (!BM_ISSET(lb->x_equivs[x], i)) {
                continue;
            }
            if (BM_ISSET(lb->halfs[0].paired, i)) {
                continue;
            }
            if (union_count == 0) {
                memcpy(bs_union, lb->halfs[0].bs[i], sizeof(bs_union));
            } else {
                BM_ANDEQ(bs_union, lb->halfs[0].bs[i]);
            }
            union_count++;
        }
        if (union_count < 2) {
            // Would be handled by lb_deduce_unique
            continue;
        }
        size_t c = lb->size - BM_POPCOUNT(bs_union);
        assert(c >= union_count);
        if (c == union_count) {
            for (size_t i = lb->x_root[x]; i < lb->size; i++) {
                if (BM_ISSET(lb->halfs[0].paired, i)) {
                    continue;
                }
                lb_mark_positive(lb, i, BM_FFZ(lb->halfs[0].bs[i]));
            }
            
        }
    }
}

void
lb_deduce(struct lb * lb) {
    while (lb->dirty) {
        lb->dirty = false;

        lb_deduce_unique(lb, 0);
        if (lb->dirty) continue;
        lb_deduce_unique(lb, 1);
        if (lb->dirty) continue;

        lb_deduce_clique(lb, 0);
        if (lb->dirty) continue;
        lb_deduce_clique(lb, 1);
        if (lb->dirty) continue;

        lb_deduce_equivalents(lb);
        if (lb->dirty) continue;
    }
}

#ifdef LB_DEBUG
static void
lb_selftest_print(struct lb * lb, const char * name) {
    if (name != NULL) {
        printf(">>> %s\n", name);
        lb_deduce(lb);
    }

    printf("     ");
    for (size_t i = 0; i < lb->size; i++) {
        printf("%c", 'A' + (char) (i % 26));
    }
    printf("     ");
    for (size_t i = 0; i < lb->size; i++) {
        printf("%c", '0' + (char) (i % 10));
    }
    printf("\n");

    for (size_t i = 0; i < lb->size; i++) {
        for (int h = 0; h < 2; h++) {
            if (h == 0) {
                printf("   %c ", '0' + (char) (i % 10));
            } else {
                printf("   %c ", 'A' + (char) (i % 26));
            }
            for (size_t j = 0; j < lb->size; j++) {
                if (BM_ISSET(lb->halfs[h].bs[i], j)) {
                    printf("x");
                } else if (BM_ISSET(lb->halfs[h].paired, i)) {
                    assert(lb->halfs[h].pair[i] == j);
                    printf("@");
                } else {
                    printf(".");
                }
            }
        }
        printf("\n");
    }
    printf("\n");
}

void
lb_selftest() {
    struct lb lb[1];
    lb_init(lb, 6);
    lb_selftest_print(lb, "init");

    lb_mark_negative(lb, 0, 1);
    lb_selftest_print(lb, "-A1");

    lb_mark_positive(lb, 2, 3);
    lb_selftest_print(lb, "+C3");

    lb_mark_negative(lb, 0, 2);
    lb_mark_negative(lb, 0, 4);
    lb_mark_negative(lb, 0, 5);
    lb_selftest_print(lb, "+A0");

    lb_mark_positive(lb, 1, 1);
    lb_mark_positive(lb, 3, 5);
    lb_mark_negative(lb, 4, 4);
    lb_selftest_print(lb, "Finish");

    lb_init(lb, 6);
    lb_mark_negative(lb, 2, 0);
    lb_mark_negative(lb, 2, 1);
    lb_mark_negative(lb, 3, 0);
    lb_mark_negative(lb, 3, 1);
    lb_mark_negative(lb, 4, 0);
    lb_mark_negative(lb, 4, 1);
    lb_mark_negative(lb, 5, 0);
    lb_mark_negative(lb, 5, 1);
    lb_selftest_print(lb, "Clique 2x4");

    lb_init(lb, 6);
    lb_mark_negative(lb, 3, 0);
    lb_mark_negative(lb, 3, 1);
    lb_mark_negative(lb, 3, 2);
    lb_mark_negative(lb, 4, 0);
    lb_mark_negative(lb, 4, 1);
    lb_mark_negative(lb, 4, 2);
    lb_mark_negative(lb, 5, 0);
    lb_mark_negative(lb, 5, 1);
    lb_mark_negative(lb, 5, 2);
    lb_mark_negative(lb, 2, 0);
    lb_mark_negative(lb, 2, 1);
    lb_selftest_print(lb, "Clique 3x3");

    lb_init(lb, 6);
    for (size_t i = 2; i < lb->size; i++) {
        for (size_t j = 0; j < i && j < 4; j++) {
            lb_mark_negative(lb, j, i);
        }
    }
    lb_selftest_print(lb, NULL);
    lb_selftest_print(lb, "diagonal on 6");

    lb_mark_negative(lb, 0, 0);
    lb_mark_negative(lb, 4, 4);
    lb_selftest_print(lb, "full solve on 6");

    // 5x5 tests
    lb_init(lb, 5);
    lb_mark_negative(lb, 2, 0);
    lb_mark_negative(lb, 2, 1);
    lb_mark_negative(lb, 3, 0);
    lb_mark_negative(lb, 3, 1);
    lb_mark_negative(lb, 4, 0);
    lb_mark_negative(lb, 4, 1);
    lb_selftest_print(lb, "Clique 2x3");

    lb_init(lb, 5);
    for (size_t i = 2; i < lb->size; i++) {
        for (size_t j = 0; j < i && j < 4; j++) {
            lb_mark_negative(lb, j, i);
        }
    }
    lb_selftest_print(lb, NULL);
    lb_selftest_print(lb, "diagonal on 5");

    lb_mark_negative(lb, 0, 0);
    lb_selftest_print(lb, "full solve on 5");

    // 6x6 with equivs
    lb_init(lb, 6);
    lb_init_equivalent(lb, 0, 1);
    lb_init_equivalent(lb, 0, 2);
    lb_init_equivalent(lb, 0, 3);
    lb_init_equivalent(lb, 0, 4);
    lb_selftest_print(lb, "Init");

    lb_mark_negative(lb, 0, 1);
    lb_selftest_print(lb, "Solved");

    lb_init(lb, 6);
    lb_init_equivalent(lb, 0, 1);
    lb_init_equivalent(lb, 0, 2);
    lb_init_equivalent(lb, 0, 3);
    lb_init_equivalent(lb, 0, 4);
    lb_init_equivalent(lb, 0, 5);
    lb_selftest_print(lb, "Fully solved");
    return;

    // 256x256 test
    lb_init(lb, 256);
    for (size_t i = 3; i < lb->size; i++) {
        for (size_t j = 0; j < i && j < lb->size - 3; j++) {
            lb_mark_negative(lb, j, i);
        }
    }
    lb_deduce(lb);
    //lb_selftest_print(lb, "diagonal on 256x256");
}

#endif
