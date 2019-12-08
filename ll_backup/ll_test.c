#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "ll.h"

static bool debug = true;

struct node {
    struct node * next;
    struct node * prev;
    uint64_t bit;
    char name[4];
};

uint64_t list_bits(struct node * list) {
    uint64_t bits = list->bit;
    if (debug) { printf("%s", list->name); }
    for (struct node * n = list->next; n != list; n = n->next) {
        bits |= n->bit;
        if (debug) { printf(" -> %s", n->name); }
    }
    if (debug) { printf(" = %#lx\n", bits); }
    return bits;
}

void nodes_init(struct node (*nodes_ptr)[64]) {
    struct node * nodes = *nodes_ptr;
    for (size_t i = 0; i < 64; i++) {
        LL_INIT(&nodes[i]);
        nodes[i].bit = (1ull << i);
        snprintf(nodes[i].name, sizeof(nodes[i].name), "%zu", i);
    }
}

int main(void) {
    struct node nodes[64];

    {
        nodes_init(&nodes);
        for (size_t i = 0; i < 64; i++) {
            assert(list_bits(&nodes[i]) == (1ull << i));
        }
    }
    
    {
        nodes_init(&nodes);
        for (size_t i = 1; i < 64; i++) {
            LL_PUSH(&nodes[0], &nodes[i]);
        }
        assert(list_bits(&nodes[0]) == -1ull);
    }

    {
        nodes_init(&nodes);

        LL_UNION(&nodes[0], &nodes[0]);
        assert(list_bits(&nodes[0]) == 0x01);

        LL_UNION(&nodes[0], &nodes[1]);
        assert(list_bits(&nodes[0]) == 0x03);

        LL_UNION(&nodes[0], &nodes[1]);
        assert(list_bits(&nodes[0]) == 0x03);

        LL_UNION(&nodes[0], &nodes[2]);
        assert(list_bits(&nodes[0]) == 0x07);

        LL_UNION(&nodes[0], &nodes[3]);
        assert(list_bits(&nodes[0]) == 0x0F);

        LL_UNION(&nodes[1], &nodes[3]);
        assert(list_bits(&nodes[0]) == 0x0F);

        LL_UNION(&nodes[2], &nodes[3]);
        assert(list_bits(&nodes[0]) == 0x0F);

        LL_UNION(&nodes[4], &nodes[5]);
        assert(list_bits(&nodes[4]) == 0x30);

        LL_UNION(&nodes[0], &nodes[5]);
        assert(list_bits(&nodes[0]) == 0x3F);

        LL_UNION(&nodes[8], &nodes[9]);
        assert(list_bits(&nodes[8]) == 0x300);

        LL_UNION(&nodes[8], &nodes[10]);
        assert(list_bits(&nodes[8]) == 0x700);

        LL_UNION(&nodes[0], &nodes[8]);
        assert(list_bits(&nodes[0]) == 0x73F);

        LL_UNION(&nodes[6], &nodes[7]);
        LL_UNION(&nodes[6], &nodes[11]);
        assert(list_bits(&nodes[6]) == 0x8C0);

        LL_UNION(&nodes[0], &nodes[6]);
        assert(list_bits(&nodes[0]) == 0xFFF);

        LL_UNION(&nodes[0], &nodes[6]);
        assert(list_bits(&nodes[0]) == 0xFFF);

        LL_UNION(&nodes[0], &nodes[11]);
        assert(list_bits(&nodes[0]) == 0xFFF);

        LL_UNION(&nodes[0], &nodes[7]);
        assert(list_bits(&nodes[0]) == 0xFFF);

        LL_UNION(&nodes[5], &nodes[7]);
        assert(list_bits(&nodes[0]) == 0xFFF);

        LL_UNION(&nodes[7], &nodes[5]);
        assert(list_bits(&nodes[0]) == 0xFFF);
    }

    {
        //debug = false;
        srand(0);
        for (size_t i = 0; i < 100000; i++) {
            nodes_init(&nodes);
            uint64_t last_bits = 0;
            for (size_t j = 0; j < 1000; j++) {
                size_t a = rand() & 63;
                size_t b = rand() & 63;
                printf("Join %zu %zu\n", a, b);
                LL_UNION(&nodes[a], &nodes[b]);
                uint64_t new_bits = list_bits(&nodes[0]);
                assert((last_bits & ~new_bits) == 0);
                last_bits = new_bits;
            }
            assert(last_bits > 1);
        }
    }

    printf("Success; all tests passed!\n");
    return 0;
}
