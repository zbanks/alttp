#pragma once
#include "ap_map.h"
#include <stddef.h>
#include <stdint.h>

#define N_ITEMS ((size_t) 216)

struct ap_item {
    size_t index;
};

struct ap_item_loc {
    size_t index;
    struct ap_node * node;
};

void
ap_item_update(void);

struct ap_item_loc *
ap_item_loc_add(struct ap_node * node);

void
ap_item_print_state(void);
