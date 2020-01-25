#pragma once
#include "ap_map.h"
#include <stddef.h>
#include <stdint.h>

enum ap_dungeon_item {
    DUNGEON_ITEM_SKEY = 1,
    DUNGEON_ITEM_BKEY,
    DUNGEON_ITEM_MAP,
    DUNGEON_ITEM_COMPASS,
};

struct ap_item {
    size_t index;
    size_t copies;
    size_t acquired;

    // Optional, -1 if invalid
    enum ap_dungeon dungeon;
    enum ap_dungeon_item dungeon_item;
    enum ap_inventory inventory;

    const char name[64];
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

