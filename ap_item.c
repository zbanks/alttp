#include "ap_item.h"
#include "ap_snes.h"
#include "lb.h"

#define N_ITEMS ((size_t) 216)

static struct ap_item items[] = {
    { .copies = 1,  .name = "1 Arrow", },
    { .copies = 10, .name = "Heart Container", },
    { .copies = 7,  .name = "50 Rupees", },
    { .copies = 4,  .name = "5 Rupees", },
    { .copies = 1,  .name = "Half Magic", },
    { .copies = 1,  .name = "Sanctuary Heart Container", },
    { .copies = 1,  .name = "100 Rupees", },
    { .copies = 2,  .name = "1 Rupee", },
    { .copies = 24, .name = "Heart Piece", },
    { .copies = 12, .name = "10 Arrows", },
    { .copies = 1,  .name = "10 Bombs", },
    { .copies = 16, .name = "3 Bombs", },
    { .copies = 5,  .name = "300 Rupees", },
    { .copies = 28, .name = "20 Rupees", },
#define X(i, n) \
    { .inventory = CONCAT(INVENTORY_, n), .name = STRINGIFY(n), },
INVENTORY_LIST
#undef X
#define X(n, abbr) \
    { .dungeon = CONCAT(DUNGEON_, n), .dungeon_item = DUNGEON_ITEM_SKEY, .name = abbr " skey", }, \
    { .dungeon = CONCAT(DUNGEON_, n), .dungeon_item = DUNGEON_ITEM_BKEY, .name = abbr " bkey", }, \
    { .dungeon = CONCAT(DUNGEON_, n), .dungeon_item = DUNGEON_ITEM_MAP,  .name = abbr " map", }, \
    { .dungeon = CONCAT(DUNGEON_, n), .dungeon_item = DUNGEON_ITEM_COMPASS, .name = abbr " comp", },
DUNGEON_LIST
#undef X
};

// items x item_locs
static struct lb item_lb;

static void
ap_item_init() {
    lb_init(&item_lb, N_ITEMS);
    size_t index = 0;

    for (size_t i = 0; i < ARRAYLEN(items); i++) {
        struct ap_item * item = &items[i];

        switch (item->inventory) {
#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wswitch"
        case 0:
#pragma GCC diagnostic pop 
            item->inventory = -1;
            break;
        case INVENTORY_SWORD:
        case INVENTORY_BOTTLES:
            item->copies = 4;
            break;
        case INVENTORY_SHIELD:
            item->copies = 3;
            break;
        case INVENTORY_GLOVES:
        case INVENTORY_BOW:
        case INVENTORY_BOOMERANG:
            item->copies = 2;
            break;
        case INVENTORY_BOMBS:
            item->index = -1;
            break;
        default: 
            item->copies = 1;
            break;
        }

        switch (item->dungeon_item) {
#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wswitch"
        case 0:
#pragma GCC diagnostic pop 
            item->dungeon_item = -1;
            item->dungeon = -1;
            break;
        case DUNGEON_ITEM_SKEY:
            switch(item->dungeon) {
            case DUNGEON_AGAHANIMS_TOWER: item->copies = 2; break;
            case DUNGEON_GANONS_TOWER: item->copies = 4; break;
            case DUNGEON_PALACE_OF_DARKNESS: item->copies = 6; break;
            case DUNGEON_SWAMP_PALACE: item->copies = 1; break;
            case DUNGEON_SKULL_WOODS: item->copies = 3; break;
            case DUNGEON_THEIVES_TOWN: item->copies = 1; break;
            case DUNGEON_ICE_PALACE: item->copies = 2; break;
            case DUNGEON_MISERY_MIRE: item->copies = 3; break;
            case DUNGEON_TURTLE_ROCK: item->copies = 4; break;
            case DUNGEON_HYRULE_CASTLE: item->copies = 0; break;
            case DUNGEON_SEWERS: item->copies = 1; break;
            case DUNGEON_EASTERN_PALACE: item->copies = 0; break;
            case DUNGEON_DESERT_PALACE: item->copies = 1; break;
            case DUNGEON_TOWER_OF_HERA: item->copies = 1; break;
            default: assert(0);
            }
            break;
        case DUNGEON_ITEM_BKEY:
            switch(item->dungeon) {
            case DUNGEON_HYRULE_CASTLE:
            case DUNGEON_AGAHANIMS_TOWER:
            case DUNGEON_SEWERS:
                item->index= -1;
                break;
            default:
                item->copies = 1;
                break;
            }
            break;
        case DUNGEON_ITEM_MAP:
            switch(item->dungeon) {
            case DUNGEON_HYRULE_CASTLE:
            case DUNGEON_AGAHANIMS_TOWER:
                item->index = -1;
                break;
            default:
                item->copies = 1;
                break;
            }
            break;
        case DUNGEON_ITEM_COMPASS:
            switch(item->dungeon) {
            case DUNGEON_HYRULE_CASTLE:
            case DUNGEON_AGAHANIMS_TOWER:
            case DUNGEON_SEWERS:
                item->index = -1;
                break;
            default:
                item->copies = 1;
                break;
            }
            break;
        }

        // Skip over the item
        if (item->index == (size_t) -1) {
            continue;
        }

        item->index = index++;
        for (size_t c = 1; c < item->copies; c++) {
            lb_init_equivalent(&item_lb, item->index, index++);
        }
    }
    assert(index == N_ITEMS);
}

void
ap_item_update() {
    static bool initialized = false;
    if (!initialized) {
        ap_item_init();
        initialized = true;
    }
}

void
ap_item_print_state() {

}
