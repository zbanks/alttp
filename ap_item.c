#include "ap_item.h"
#include "ap_map.h"
#include "ap_snes.h"
#include "ap_macro.h"
#include "lb.h"
#include "bm.h"

// 216 randomized locations + 10 crystals/pendants
#define N_ITEMS ((size_t) (216 + 10))

static struct ap_item items[] = {
    { .copies = 1, .is_progression = true, .name = "Green Pendant", .type = MISC_ITEM_GREEN_PENDANT, },
    { .copies = 2, .is_progression = true, .name = "Red/Blue Pendant", .type = MISC_ITEM_REDBLUE_PENDANT, },
    { .copies = 7, .is_progression = true, .name = "Crystal", .type = MISC_ITEM_CRYSTAL, },
#define X(n, abbr) \
    { .dungeon = CONCAT(DUNGEON_, n), .type = DUNGEON_ITEM_SKEY, .is_progression = true, .name = abbr " skey", }, \
    { .dungeon = CONCAT(DUNGEON_, n), .type = DUNGEON_ITEM_BKEY, .is_progression = true, .name = abbr " bkey", }, \
    { .dungeon = CONCAT(DUNGEON_, n), .type = DUNGEON_ITEM_MAP,  .name = abbr " map", }, \
    { .dungeon = CONCAT(DUNGEON_, n), .type = DUNGEON_ITEM_COMPASS, .name = abbr " comp", },
DUNGEON_LIST
#undef X
#define X(i, n) \
    { .copies = 1, .type = CONCAT(INVENTORY_, n), .is_progression = true, .name = STRINGIFY(n), },
INVENTORY_LIST
#undef X
    { .copies = 1,  .name = "Half Magic", .type = MISC_ITEM_MAGIC_UPGRADE, },
    { .copies = 24, .name = "Heart Piece", .type = MISC_ITEM_HEART_PIECE, },
    { .copies = 11, .name = "Heart Cont.", .type = MISC_ITEM_HEART_CONTAINER, }, //{ .copies = 1,  .name = "S. Heart Cont.", },
    { .copies = 2,  .name = "1 Rupee", .type = MISC_ITEM_RUPEES_1, },
    { .copies = 4,  .name = "5 Rupees", .type = MISC_ITEM_RUPEES_5, },
    { .copies = 28, .name = "20 Rupees", .type = MISC_ITEM_RUPEES_20, },
    { .copies = 7,  .name = "50 Rupees", .type = MISC_ITEM_RUPEES_50, },
    { .copies = 1,  .name = "100 Rupees", .type = MISC_ITEM_RUPEES_100, },
    { .copies = 5,  .name = "300 Rupees", .type = MISC_ITEM_RUPEES_300, },
    { .copies = 1,  .name = "1 Arrow", .type = MISC_ITEM_ARROWS_1, },
    { .copies = 12, .name = "10 Arrows", .type = MISC_ITEM_ARROWS_10, },
    { .copies = 16, .name = "3 Bombs", .type = MISC_ITEM_BOMBS_3, },
    { .copies = 1,  .name = "10 Bombs", .type = MISC_ITEM_BOMBS_10, },
};

// TODO: map multiple items correctly; rupees/bombs/arrows/magic
// From alttpr Item.php
static int recv_item_map [0xB0] = {
    INVENTORY_SWORD,                    // 0x00 L1SwordAndShield
    INVENTORY_SWORD,                    // 0x01 L2Sword
    INVENTORY_SWORD,                    // 0x02 L3Sword
    INVENTORY_SWORD,                    // 0x03 L4Sword
    INVENTORY_SHIELD,                   // 0x04 BlueShield
    INVENTORY_SHIELD,                   // 0x05 RedShield
    INVENTORY_SHIELD,                   // 0x06 MirrorShield
    INVENTORY_FIRE_ROD,                 // 0x07 FireRod
    INVENTORY_ICE_ROD,                  // 0x08 IceRod
    INVENTORY_HAMMER,                   // 0x09 Hammer
    INVENTORY_HOOKSHOT,                 // 0x0A Hookshot
    INVENTORY_BOW,                      // 0x0B Bow
    INVENTORY_BOOMERANG,                // 0x0C Boomerang
    INVENTORY_POWDER,                   // 0x0D Powder
    INVENTORY_BOTTLE,                   // 0x0E Bee
    INVENTORY_BOMBOS,                   // 0x0f Bombos
    INVENTORY_ETHER,                    // 0x10 Ether
    INVENTORY_QUAKE,                    // 0x11 Quake
    INVENTORY_LAMP,                     // 0x12 Lamp
    INVENTORY_FLUTE,                    // 0x13 Shovel [XXX]
    INVENTORY_FLUTE,                    // 0x14 OcarinaInactive
    INVENTORY_RED_CANE,                 // 0x15 CaneOfSomaria
    INVENTORY_BOTTLE,                   // 0x16 Bottle
    MISC_ITEM_HEART_PIECE,              // 0x17 PieceOfHeart
    INVENTORY_BLUE_CANE,                // 0x18 CaneOfByrna
    INVENTORY_MAGIC_CAPE,               // 0x19 Cape
    INVENTORY_MIRROR,                   // 0x1A MagicMirror
    INVENTORY_GLOVES,                   // 0x1B PowerGlove
    INVENTORY_GLOVES,                   // 0x1C TitansMitt
    INVENTORY_BOOK,                     // 0x1D BookOfMudora
    INVENTORY_FLIPPERS,                 // 0x1E Flippers
    INVENTORY_MOON_PEARL,               // 0x1F MoonPearl
    0,                                  // [0x20 Crystal???]
    INVENTORY_NET,                      // 0x21 BugCatchingNet
    INVENTORY_ARMOR,                    // 0x22 BlueMail
    INVENTORY_ARMOR,                    // 0x23 RedMail
    DUNGEON_ITEM_SKEY,                  // 0x24 Key
    DUNGEON_ITEM_COMPASS,               // 0x25 Compass
    MISC_ITEM_HEART_CONTAINER,          // 0x26 HeartContainerNoAnimation
    MISC_ITEM_BOMBS_1,                  // 0x27 Bomb
    MISC_ITEM_BOMBS_3,                  // 0x28 ThreeBombs
    INVENTORY_POWDER,                   // 0x29 Mushroom [XXX]
    INVENTORY_BOOMERANG,                // 0x2A RedBoomerang
    INVENTORY_BOTTLE,                   // 0x2B BottleWithRedPotion
    INVENTORY_BOTTLE,                   // 0x2C BottleWithGreenPotion
    INVENTORY_BOTTLE,                   // 0x2D BottleWithBluePotion
    MISC_ITEM_POTION_RED,               // 0x2E RedPotion
    MISC_ITEM_POTION_GREEN,             // 0x2F GreenPotion
    MISC_ITEM_POTION_BLUE,              // 0x30 BluePotion
    MISC_ITEM_BOMBS_10,                 // 0x31 TenBombs
    DUNGEON_ITEM_BKEY,                  // 0x32 BigKey
    DUNGEON_ITEM_MAP,                   // 0x33 Map
    MISC_ITEM_RUPEES_1,                 // 0x34 OneRupee
    MISC_ITEM_RUPEES_5,                 // 0x35 FiveRupees
    MISC_ITEM_RUPEES_20,                // 0x36 TwentyRupees
    MISC_ITEM_GREEN_PENDANT,            // 0x37 PendantOfCourage
    MISC_ITEM_REDBLUE_PENDANT,          // 0x38 PendantOfWisdom
    MISC_ITEM_REDBLUE_PENDANT,          // 0x39 PendantOfPower
    INVENTORY_BOW,                      // 0x3A BowAndArrows
    INVENTORY_BOW,                      // 0x3B BowAndSilverArrows
    INVENTORY_BOTTLE,                   // 0x3C BottleWithBee
    INVENTORY_BOTTLE,                   // 0x3D BottleWithFairy
    MISC_ITEM_HEART_CONTAINER,          // 0x3E BossHeartContainer
    MISC_ITEM_HEART_CONTAINER,          // 0x3F HeartContainer
    MISC_ITEM_RUPEES_100,               // 0x40 OneHundredRupees
    MISC_ITEM_RUPEES_50,                // 0x41 FiftyRupees
    MISC_ITEM_HEART_SMALL,              // 0x42 Heart
    MISC_ITEM_ARROWS_1,                 // 0x43 Arrow
    MISC_ITEM_ARROWS_10,                // 0x44 TenArrows
    MISC_ITEM_MAGIC_SMALL,              // 0x45 SmallMagic
    MISC_ITEM_RUPEES_300,               // 0x46 ThreeHundredRupees
    MISC_ITEM_RUPEES_20,                // 0x47 TwentyRupees2
    INVENTORY_BOTTLE,                   // 0x48 BottleWithGoldBee
    INVENTORY_SWORD,                    // 0x49 L1Sword
    INVENTORY_FLUTE,                    // 0x4A OcarinaActive
    INVENTORY_BOOTS,                    // 0x4B PegasusBoots
    MISC_ITEM_BOMB_UPGRADE,             // 0x4C BombUpgrade50
    MISC_ITEM_ARROW_UPGRADE,            // 0x4D ArrowUpgrade70
    MISC_ITEM_MAGIC_UPGRADE,            // 0x4E HalfMagic
    MISC_ITEM_MAGIC_UPGRADE,            // 0x4F QuarterMagic
    INVENTORY_SWORD,                    // 0x50 MasterSword
    MISC_ITEM_BOMB_UPGRADE,             // 0x51 BombUpgrade5
    MISC_ITEM_BOMB_UPGRADE,             // 0x52 BombUpgrade10
    MISC_ITEM_BOMB_UPGRADE,             // 0x53 ArrowUpgrade5
    MISC_ITEM_BOMB_UPGRADE,             // 0x54 ArrowUpgrade10
    MISC_ITEM_BOMB_UPGRADE,             // 0x55 Programmable1
    0,                                  // 0x56 Programmable2
    0,                                  // 0x57 Programmable3
    INVENTORY_BOW,                      // 0x58 SilverArrowUpgrade
    MISC_ITEM_RUPEES_NEGATIVE,          // 0x59 Rupoor
    0,                                  // 0x5A Nothing
    0,                                  // 0x5B RedClock
    0,                                  // 0x5C BlueClock
    0,                                  // 0x5D GreenClock
    INVENTORY_SWORD,                    // 0x5E ProgressiveSword
    INVENTORY_SHIELD,                   // 0x5F ProgressiveShield
    INVENTORY_ARMOR,                    // 0x60 ProgressiveArmor
    INVENTORY_GLOVES,                   // 0x61 ProgressiveGlove
    0,                                  // 0x62 singleRNG
    0,                                  // 0x63 multiRNG
    INVENTORY_BOW,                      // 0x64 ProgressiveBow
    INVENTORY_BOW,                      // 0x65 ProgressiveBow
    INVENTORY_BOW,                      // 0x65 ProgressiveBowAlternate
    0,                                  // 0x6A Triforce
    0,                                  // 0x6B PowerStar
    0,                                  // 0x6C TriforcePiece
    // 0x70 MapLW
    // 0x71 MapDW
    // 0x72 MapA2
    // 0x73 MapD7
    // 0x74 MapD4
    // 0x75 MapP3
    // 0x76 MapD5
    // 0x77 MapD3
    // 0x78 MapD6
    // 0x79 MapD1
    // 0x7A MapD2
    // 0x7B MapA1
    // 0x7C MapP2
    // 0x7D MapP1
    // 0x7E MapH1
    // 0x7F MapH2
    [0x70 ... 0x7F] = DUNGEON_ITEM_MAP,
    // 0x82 CompassA2
    // 0x83 CompassD7
    // 0x84 CompassD4
    // 0x85 CompassP3
    // 0x86 CompassD5
    // 0x87 CompassD3
    // 0x88 CompassD6
    // 0x89 CompassD1
    // 0x8A CompassD2
    // 0x8B CompassA1
    // 0x8C CompassP2
    // 0x8D CompassP1
    // 0x8E CompassH1
    // 0x8F CompassH2
    [0x80 ... 0x8F] = DUNGEON_ITEM_COMPASS,
    // 0x92 BigKeyA2
    // 0x93 BigKeyD7
    // 0x94 BigKeyD4
    // 0x95 BigKeyP3
    // 0x96 BigKeyD5
    // 0x97 BigKeyD3
    // 0x98 BigKeyD6
    // 0x99 BigKeyD1
    // 0x9A BigKeyD2
    // 0x9B BigKeyA1
    // 0x9C BigKeyP2
    // 0x9D BigKeyP1
    // 0x9E BigKeyH1
    // 0x9F BigKeyH2
    [0x90 ... 0x9F] = DUNGEON_ITEM_BKEY,
    // 0xA0 KeyH2
    // 0xA1 KeyH1
    // 0xA2 KeyP1
    // 0xA3 KeyP2
    // 0xA4 KeyA1
    // 0xA5 KeyD2
    // 0xA6 KeyD1
    // 0xA7 KeyD6
    // 0xA8 KeyD3
    // 0xA9 KeyD5
    // 0xAA KeyP3
    // 0xAB KeyD4
    // 0xAC KeyD7
    // 0xAD KeyA2
    // 0xAF KeyGK
    [0xA0 ... 0xAF] = DUNGEON_ITEM_SKEY,
    // null BigRedBomb
    // null Crystal1
    // null Crystal2
    // null Crystal3
    // null Crystal4
    // null Crystal5
    // null Crystal6
    // null Crystal7
    // null DefeatAgahnim
    // null DefeatAgahnim2
    // null DefeatGanon
    // null RescueZelda
};

static const struct ap_item *item_by_index[N_ITEMS];
static const struct ap_item *item_by_type[_MISC_ITEM_MAX];
static const struct ap_item *item_by_dungeon_item[_DUNGEON_MAX][_DUNGEON_ITEM_MAX];

// items x item_locs
static struct lb item_lb;

enum area {
#define X(name, abbr) CONCAT(AREA_, name),
DUNGEON_LIST
#undef X
    AREA_LIGHT_OVERWORLD,
    AREA_DARK_OVERWORLD,
    AREA_INSIDE,
    _AREA_MAX,
};

static const size_t area_loc_count[_AREA_MAX] = {
    [AREA_SEWERS] = 4,
    [AREA_HYRULE_CASTLE] = 6, // idk
    [AREA_EASTERN_PALACE] = 8,
    [AREA_DESERT_PALACE] = 7,
    [AREA_AGAHANIMS_TOWER] = 2,
    [AREA_SWAMP_PALACE] = 11,
    [AREA_PALACE_OF_DARKNESS] = 15,
    [AREA_MISERY_MIRE] = 9,
    [AREA_SKULL_WOODS] = 9,
    [AREA_ICE_PALACE] = 9,
    [AREA_TOWER_OF_HERA] = 7,
    [AREA_THEIVES_TOWN] = 9,
    [AREA_TURTLE_ROCK] = 13,
    [AREA_GANONS_TOWER] = 25,

    // The following are very very rough estimates
    [AREA_LIGHT_OVERWORLD] = 8,
    [AREA_DARK_OVERWORLD] = 6,
    [AREA_INSIDE] = 78,
};

static size_t area_loc_start[_AREA_MAX] = {0};

static struct ap_item_loc item_locs[N_ITEMS];
static struct ap_item_loc * item_locs_by_item[N_ITEMS];

static void
ap_item_init() {
    lb_init(&item_lb, N_ITEMS);
    size_t index = 0;

    for (size_t i = 0; i < ARRAYLEN(items); i++) {
        struct ap_item * item = &items[i];

        switch (item->type) {
        case 0:
            assert(0);
            break;
        case INVENTORY_SWORD:
        case INVENTORY_BOTTLE:
            item->copies = 4;
            break;
        case INVENTORY_SHIELD:
            item->copies = 3;
            break;
        case INVENTORY_GLOVES:
        case INVENTORY_BOW:
        case INVENTORY_BOOMERANG:
        case INVENTORY_ARMOR:
        case INVENTORY_FLUTE:   // XXX actually FLUTE/SHOVEL
        case INVENTORY_POWDER:  // XXX actually POWDER/MUSHROOM
            item->copies = 2;
            break;
        case INVENTORY_BOMBS:
            item->index = -1;
            break;
        default: 
            break;
        }

        switch (item->type) {
        case _INVENTORY_MIN ... (_INVENTORY_MAX-1):
        case _MISC_ITEM_MIN ... (_MISC_ITEM_MAX-1):
            item->dungeon = -1;
            break;
        case DUNGEON_ITEM_SKEY:
            switch(item->dungeon) {
            // This only counts randomized keys; not keys dropped by enemies
            case DUNGEON_AGAHANIMS_TOWER: item->copies = 2; break;
            case DUNGEON_GANONS_TOWER: item->copies = 4; break;
            case DUNGEON_PALACE_OF_DARKNESS: item->copies = 6; break;
            case DUNGEON_SWAMP_PALACE: item->copies = 1; break;
            case DUNGEON_SKULL_WOODS: item->copies = 3; break;
            case DUNGEON_THEIVES_TOWN: item->copies = 1; break;
            case DUNGEON_ICE_PALACE: item->copies = 2; break;
            case DUNGEON_MISERY_MIRE: item->copies = 3; break;
            case DUNGEON_TURTLE_ROCK: item->copies = 4; break;
            case DUNGEON_HYRULE_CASTLE: item->index = -1; break;
            case DUNGEON_SEWERS: item->copies = 1; break;
            case DUNGEON_EASTERN_PALACE: item->index = -1; break;
            case DUNGEON_DESERT_PALACE: item->copies = 1; break;
            case DUNGEON_TOWER_OF_HERA: item->copies = 1; break;
            default: assert(0);
            }
            break;
        case DUNGEON_ITEM_BKEY:
            switch(item->dungeon) {
            // HC has a big key but it's a guaranteed drop
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
        case DUNGEON_ITEM_MAP:
            switch(item->dungeon) {
            case DUNGEON_SEWERS:    // XXX: Sewers vs. HC?
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
        /*
        case DUNGEON_ITEM_REWARD:
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
        */
        default:
            assert(0);
        }

        // Skip over the item
        if (item->index == (size_t) -1) {
            continue;
        }

        item_by_index[index] = item;
        item->index = index++;
        for (size_t c = 1; c < item->copies; c++) {
            item_by_index[index] = item;
            lb_init_equivalent(&item_lb, item->index, index++);
        }

        if (ITEM_IS_DUNGEON(item->type)) {
            assert(item_by_dungeon_item[item->dungeon][item->type] == NULL);
            item_by_dungeon_item[item->dungeon][item->type] = item;
        } else {
            LOG("%zu %p", index, item);
            assert(item_by_type[item->type] == NULL);
            item_by_type[item->type] = item;
        }


        LOG("%5zu %s * %zu", item->index, item->name, item->copies);
    }
    assert(index == N_ITEMS);

    for (size_t i = 0; i < N_ITEMS; i++) {
        item_locs[i].dungeon = (enum ap_dungeon) -1;
    }

    size_t l = 0;
    for (enum area area = 0; area < _AREA_MAX; area++) {
        assert_bp(area_loc_count[area] > 0);
        assert_bp(l <= N_ITEMS);

        area_loc_start[area] = l;
        for (size_t dl = 0; dl < area_loc_count[area]; dl++) {
            item_locs[l++].dungeon = area;
        }
    }
    LOG("l = %zu", l);
    assert_bp(l == N_ITEMS);

    size_t real_dungeons = 0;
    for (size_t i = 0; i < ARRAYLEN(items); i++) {
        const struct ap_item * item = &items[i];
        if (item->dungeon == (enum ap_dungeon) -1 || item->index == (size_t) -1) {
            continue;
        }
        if (item->dungeon == DUNGEON_AGAHANIMS_TOWER) {
            //continue;
        }

        // Dungeon items (e.g. keys, map) must be in the dungeon they are for
        enum area a = item->dungeon;
        LOG("x'ing %s*%zu %zu %zu", item->name, item->copies, area_loc_start[a], area_loc_count[a]);
        for (size_t l = 0; l < N_ITEMS; l++) {
            if (l < area_loc_start[a] || l >= area_loc_start[a] + area_loc_count[a]) {
                lb_mark_negative(&item_lb, item->index, l);
            }
        }

        if (item->type == DUNGEON_ITEM_COMPASS && item->dungeon != DUNGEON_GANONS_TOWER) {
            // Dungeons with compass happen to have pendants/crystals; held in the last slot
            LOG("placing reward for %s", item->name);
            for (size_t j = 10; j < N_ITEMS; j++) {
                lb_mark_negative(&item_lb, j, area_loc_start[a] + area_loc_count[a] - 1);
            }
            real_dungeons++;
        }
    }
    assert(real_dungeons == 10);
    //ap_item_update();
    //ap_item_print_state();
}

struct ap_item_loc *
ap_item_loc_add(struct ap_node * node) {
    if (node->item_loc != NULL) {
        return node->item_loc;
    }
    enum area a = node->screen->dungeon_id;
    if (a >= (enum area) _DUNGEON_MAX) {
        if (node->screen->dungeon_room == (uint16_t) -1) {
            a = AREA_LIGHT_OVERWORLD; // XXX support dark world
        } else {
            a = AREA_INSIDE;
        }
    } else {
        if (node->type == NODE_ITEM || node->type == NODE_SPRITE) {
            // XXX items inside are constant drops, not accounted for
            return NULL;
        }
    }
    assert(a < _AREA_MAX);
    for (size_t l = area_loc_start[a]; l < area_loc_start[a] + area_loc_count[a]; l++) {
        if (item_locs[l].node == NULL) {
            node->item_loc = &item_locs[l];
            node->item_loc->index = l;
            node->item_loc->node = node;
            break;
        }
    }
    if (node->item_loc == NULL) {
        assert_bp(0);
        return NULL;
    }

    if (node->screen->id == 0x0a68 && node->type == NODE_SPRITE) {
        // Well Uncle
        ap_item_loc_set_enum(node->item_loc, INVENTORY_SWORD);
    }

    item_lb.dirty = true;
    ap_item_update();

    return node->item_loc;
}

void
ap_item_loc_set_raw(struct ap_item_loc * item_loc, uint8_t recv_item) {
    assert_bp(recv_item < ARRAYLEN(recv_item_map));
    int item_type = recv_item_map[recv_item];
    return ap_item_loc_set_enum(item_loc, item_type);
}

void
ap_item_loc_set_enum(struct ap_item_loc * item_loc, int item_type) {
    LOGB("Set item: %zu %p %d", item_loc->index, item_loc, item_type);
    assert(item_type > 0);

    if (item_loc->item != NULL) {
        assert(item_loc->item->type == item_type);
        return;
    }

    const struct ap_item * item = NULL;
    if (ITEM_IS_DUNGEON(item_type)) {
        assert(item_loc->dungeon != (enum ap_dungeon) -1);
        item = item_by_dungeon_item[item_loc->dungeon][item_type];
    } else {
        item = item_by_type[item_type];
    }
    assert(item != NULL);

    lb_mark_positive(&item_lb, item->index, item_loc->index);
    ap_item_update();
}

void
ap_item_update() {
    static bool initialized = false;
    if (!initialized) {
        ap_item_init();
        initialized = true;
    }
    if (!item_lb.dirty) {
        return;
    }
    lb_deduce(&item_lb);

    double item_progression_value[N_ITEMS] = {0};
    for (size_t i = 0; i < N_ITEMS; i++) {
        if (!item_by_index[i]->is_progression) {
            item_progression_value[i] = 0;
            continue;
        }

        size_t possible_locations;
        if (BM_ISSET(item_lb.halfs[0].paired, i)) {
            possible_locations = 1;
        } else {
            possible_locations = N_ITEMS - BM_POPCOUNT(item_lb.halfs[0].bs[i]);
            //LOG("%zu %s %zu", i, item_by_index[i]->name, possible_locations);
        }
        assert(possible_locations > 0 && possible_locations <= N_ITEMS);

        item_progression_value[i] = 100. / possible_locations;
    }

    for (size_t l = 0; l < N_ITEMS; l++) {
        struct ap_item_loc * item_loc = &item_locs[l];

        if (BM_ISSET(item_lb.halfs[1].paired, l)) {
            size_t item_index = item_lb.halfs[1].pair[l];
            const struct ap_item * item = item_by_index[item_index];
            assert(item_loc->item == NULL || item_loc->item == item);
            assert(item_locs_by_item[item_index] == NULL || item_locs_by_item[item_index] == item_loc);
            if (!item_loc->item) {
                const char *loc_name = item_loc->node != NULL ? item_loc->node->name : "(no node)";
                LOGB("Paired location %zu '%s' with item '%s'", l, loc_name, item->name);
            }
            item_locs_by_item[item_index] = item_loc;
            item_loc->item = item;
            item_loc->progression_value = item_progression_value[item_index];
        } else {
            item_loc->progression_value = 0.;
            for (size_t i = 0; i < N_ITEMS; i++) {
                if (!BM_ISSET(item_lb.halfs[1].bs[l], i)) {
                    item_loc->progression_value += item_progression_value[i];
                }
            }
        }
    }

    ap_item_print_state();
}

static void 
lb_selftest_print(struct lb * lb) {
    lb_deduce(lb);

    printf("   %20s ", "");
    for (size_t i = 0; i < lb->size; i++) {
        printf("%zx", i >> 4);
    }
    printf("\n");

    printf("   %20s ", "");
    for (size_t i = 0; i < lb->size; i++) {
        printf("%zx", i & 0xF);
    }
    printf("\n");

    for (size_t i = 0; i < lb->size; i++) {
        size_t h = 0;
        printf("   %20s ", item_by_index[i]->name);
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
        printf("\n");
    }
    printf("\n");
}

void
ap_item_print_state() {
    lb_selftest_print(&item_lb);

    for (size_t l = 0; l < N_ITEMS; l++) {
        struct ap_item_loc * item_loc = &item_locs[l];
        if (item_loc->node != NULL) {
            LOG("Loc %#6zx, Value: %lf; %s %u %s %s", l, item_loc->progression_value,
                (item_loc->item != NULL ? item_loc->item->name : "?"),
                item_loc->dungeon, item_loc->node->name, item_loc->node->screen->name);
        }
    }
}

