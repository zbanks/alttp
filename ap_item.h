#pragma once
#include "ap_map.h"
#include <stddef.h>
#include <stdint.h>

#define ITEM_IS_INVENTORY(x) ((x) >= _INVENTORY_MIN && (x) < _INVENTORY_MAX)
#define ITEM_IS_DUNGEON(x) ((x) >= _DUNGEON_ITEM_MIN && (x) < _DUNGEON_ITEM_MAX)
#define ITEM_IS_MISC(x) ((x) >= _MISC_ITEM_MIN && (x) < _MISC_ITEM_MAX)

enum ap_dungeon_item {
    _DUNGEON_ITEM_MIN = _INVENTORY_MAX,
    DUNGEON_ITEM_SKEY = _DUNGEON_ITEM_MIN,
    DUNGEON_ITEM_BKEY,
    DUNGEON_ITEM_MAP,
    DUNGEON_ITEM_COMPASS,
    DUNGEON_ITEM_REWARD, // Either Pendant/Crystal in MISC_ITEM
    _DUNGEON_ITEM_MAX
};

enum ap_misc_item {
    _MISC_ITEM_MIN = _DUNGEON_ITEM_MAX,
    MISC_ITEM_GREEN_PENDANT = _MISC_ITEM_MIN,
    MISC_ITEM_REDBLUE_PENDANT,
    MISC_ITEM_CRYSTAL,
    MISC_ITEM_HEART_PIECE,
    MISC_ITEM_HEART_CONTAINER,
    MISC_ITEM_RUPEES_NEGATIVE,
    MISC_ITEM_RUPEES_1,
    MISC_ITEM_RUPEES_5,
    MISC_ITEM_RUPEES_20,
    MISC_ITEM_RUPEES_50,
    MISC_ITEM_RUPEES_100,
    MISC_ITEM_RUPEES_300,
    MISC_ITEM_ARROWS_1,
    MISC_ITEM_ARROWS_3,
    MISC_ITEM_ARROWS_10,
    MISC_ITEM_BOMBS_1,
    MISC_ITEM_BOMBS_3,
    MISC_ITEM_BOMBS_10,
    MISC_ITEM_HEART_SMALL,
    MISC_ITEM_MAGIC_SMALL,
    MISC_ITEM_MAGIC_FULL,
    MISC_ITEM_POTION_RED,
    MISC_ITEM_POTION_GREEN,
    MISC_ITEM_POTION_BLUE,
    MISC_ITEM_MAGIC_UPGRADE,
    MISC_ITEM_BOMB_UPGRADE,
    MISC_ITEM_ARROW_UPGRADE,
    _MISC_ITEM_MAX
};

struct ap_item {
    size_t index;
    size_t copies;
    size_t acquired;

    // Optional, -1 if invalid
    enum ap_dungeon dungeon;

    // Union of ap_inventory, ap_dungeon_item, ap_misc_item
    int type; 

    // This is a rudamentary approximation
    bool is_progression;

    const char name[64];
};

struct ap_item_loc {
    size_t index;

    // If known; otherwise null
    const struct ap_item * item;

    // Optional, -1 if invalid
    enum ap_dungeon dungeon;

    struct ap_node * node;

    double progression_value;
};

void
ap_item_update(void);

struct ap_item_loc *
ap_item_loc_add(struct ap_node * node);

void
ap_item_loc_set_raw(struct ap_item_loc * item_loc, uint8_t recv_item);

void
ap_item_loc_set_enum(struct ap_item_loc * item_loc, int item_type);

double
ap_item_loc_value(struct ap_item_loc * item_loc);

void
ap_item_print_state(void);


