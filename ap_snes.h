#pragma once
#include "ap_macro.h"
#include "alttp_public.h"

#define SNES_TR_MASK		((uint16_t) (1ul <<  4))
#define SNES_TL_MASK		((uint16_t) (1ul <<  5))
#define SNES_X_MASK			((uint16_t) (1ul <<  6))
#define SNES_A_MASK			((uint16_t) (1ul <<  7))
#define SNES_RIGHT_MASK		((uint16_t) (1ul <<  8))
#define SNES_LEFT_MASK		((uint16_t) (1ul <<  9))
#define SNES_DOWN_MASK		((uint16_t) (1ul << 10))
#define SNES_UP_MASK		((uint16_t) (1ul << 11))
#define SNES_START_MASK		((uint16_t) (1ul << 12))
#define SNES_SELECT_MASK	((uint16_t) (1ul << 13))
#define SNES_Y_MASK			((uint16_t) (1ul << 14))
#define SNES_B_MASK			((uint16_t) (1ul << 15))

#define SNES_MASK(key) CONCAT(CONCAT(SNES_, key), _MASK)

#define JOYPAD_TEST(key) ((*joypad) & SNES_MASK(key))
#define JOYPAD_SET(key) ((*joypad) |= SNES_MASK(key))
#define JOYPAD_CLEAR(key) ((*joypad) &= (uint16_t) ~SNES_MASK(key))
#define JOYPAD_EVENT(btn) ({\
    static bool _debounce = false; \
    bool _ret = false; \
    if (_debounce && !JOYPAD_TEST(btn)) { \
        _debounce = false; \
    } else if (!_debounce && JOYPAD_TEST(btn)) { \
        _debounce = true; \
        _ret = true; \
    } \
    _ret; })

#define AP_RAM_LIST \
    X(module_index,         uint8_t,   0x7E0010)   \
    X(submodule_index,      uint8_t,   0x7E0011)   \
    X(frame_counter,        uint8_t,   0x7E001A)   \
    X(in_building,          uint8_t,   0x7E001B)   \
    X(area,                 uint8_t,   0x7E008A)   \
    X(link_x,               uint16_t,  0x7E0022)   \
    X(link_y,               uint16_t,  0x7E0020)   \
    X(link_dx,              uint8_t,   0x7E0031)   \
    X(link_dy,              uint8_t,   0x7E0030)   \
    X(link_hold,            uint16_t,  0x7E0308)   \
    X(link_direction,       uint8_t,   0x7E002F)   \
    X(link_swordstate,      uint8_t,   0x7E003A)   \
    X(link_state,           uint8_t,   0x7E005D)   \
    X(overworld_index,      uint16_t,  0x7E008A)   \
    X(dungeon_room,         uint16_t,  0x7E00A0)   \
    X(menu_part,            uint8_t,   0x7E00C8)   \
    X(room_state,           uint16_t,  0x7E0400)   \
    X(link_lower_level,     uint8_t,   0x7E00EE)   \
    X(dngn_open_doors,      uint16_t,  0x7E068C)   \
    X(sprite_drop,          uint8_t,   0x7E0CBA)   \
    X(sprite_y_lo,          uint8_t,   0x7E0D00)   \
    X(sprite_x_lo,          uint8_t,   0x7E0D10)   \
    X(sprite_y_hi,          uint8_t,   0x7E0D20)   \
    X(sprite_x_hi,          uint8_t,   0x7E0D30)   \
    X(sprite_state,         uint8_t,   0x7E0DD0)   \
    X(sprite_type,          uint8_t,   0x7E0E20)   \
    X(sprite_subtype1,      uint8_t,   0x7E0E30)   \
    X(sprite_subtype2,      uint8_t,   0x7E0E80)   \
    X(sprite_lower_level,   uint8_t,   0x7E0F20)   \
    X(map_area,             uint16_t,  0x7E040A)   \
    X(map_y_offset,         uint16_t,  0x7E0708)   \
    X(map_y_mask,           uint16_t,  0x7E070A)   \
    X(map_x_offset,         uint16_t,  0x7E070C)   \
    X(map_x_mask,           uint16_t,  0x7E070E)   \
    X(map_area_size,        uint16_t,  0x7E0712)   \
    X(over_map16,           uint16_t,  0x7E2000)   \
    X(dngn_bg1_map8,        uint16_t,  0x7E2000)   \
    X(dngn_bg2_map8,        uint16_t,  0x7E4000)   \
    X(map16_to_map8,        uint64_t,  0x078000)   \
    X(chr_to_tattr,         uint64_t,  0x078000)   \
    X(dngn_bg2_tattr,       uint8_t,   0x7F2000)   \
    X(dngn_bg1_tattr,       uint8_t,   0x7F3000)   \
    X(over_tattr,           uint8_t,   0x1BF110)   \
    X(over_tattr2,          uint8_t,   0x0FFD94)   \
    X(transition_dir,       uint16_t,  0x7E0410)   \
    X(layers_enabled,       uint8_t,   0x7E001C)   \
    X(vram_transfer,        uint8_t,   0x7E0019)   \
    X(entrance_ys,          uint16_t,  0x02CCBD)   \
    X(entrance_xs,          uint16_t,  0x02CDC7)   \
    X(over_ent_areas,       uint16_t,  0x1BB96F)   \
    X(over_ent_map16s,      uint16_t,  0x1BBA71)   \
    X(over_ent_ids,         uint8_t,   0x1BBB73)   \
    X(over_hle_map16s,      uint16_t,  0x1BB800)   \
    X(over_hle_areas,       uint16_t,  0x1BB826)   \
    X(over_hle_ids,         uint8_t,   0x1BB84C)   \
    X(touching_chest,       uint8_t,   0x7E02E5)   \
    X(item_recv_method,     uint8_t,   0x7E02E9)   \
    X(push_dir_bitmask,     uint8_t,   0x7E0026)   \
    X(push_timer,           uint8_t,   0x7E0371)   \
    X(carrying_bit7,        uint8_t,   0x7E0308)   \
    X(ignore_sprites,       uint8_t,   0x7E037B)   \
    X(sram_room_state,      uint16_t,  0x7EF000)   \
    X(sram_overworld_state, uint8_t,   0x7EF280)   \
    X(inventory_bombs,      uint8_t,   0x7EF343)   \
    X(health_current,       uint8_t,   0x7EF36D)   \
    X(health_capacity,      uint8_t,   0x7EF36C)   \

extern struct ap_ram {
#define X(name, type, offset) const type * name;
AP_RAM_LIST
#undef X
} ap_ram;

extern struct ap_snes9x * ap_emu;

#define TILE_ATTR_LIST \
    X(WALK) /* Walkable */ \
    X(SWIM) /* Swim-able */ \
    X(DOOR) /* Door or other transition */ \
    X(NODE) /* Importatn goal node (chest, or something under a pot, stairs) */ \
    X(LFT0) /* Can lift with no power ups */ \
    X(LFT1) /* Can lift with power glove */ \
    X(LFT2) /* Can lift with titans mitts */ \
    X(LDGE) /* Ledge */ \
    X(CHST) /* Chest */ \
    X(BONK) /* Bonk rocks */ \
    X(SWCH) /* Floor button */ \

enum {
#define X(d) CONCAT(_TILE_ATTR_INDEX_, d),
TILE_ATTR_LIST
#undef X
};
enum {
#define X(d) CONCAT(TILE_ATTR_, d) = 1ul << CONCAT(_TILE_ATTR_INDEX_, d),
TILE_ATTR_LIST
#undef X
};
static const char * const ap_tile_attr_names[] = {
#define X(d) [CONCAT(TILE_ATTR_, d)] = STRINGIFY(d),
TILE_ATTR_LIST
#undef X
};

static const uint16_t ap_tile_attrs[256] = {
    [0x00] = TILE_ATTR_WALK,
    [0x01] = 0,
    [0x02] = 0,

    [0x08] = TILE_ATTR_SWIM,
    [0x09] = TILE_ATTR_WALK,
    [0x10] = 0, // edge of ledge?
    [0x1c] = TILE_ATTR_WALK, // open below?
    [0x1d] = TILE_ATTR_WALK | TILE_ATTR_DOOR, // stairs?
    [0x1e] = TILE_ATTR_WALK | TILE_ATTR_DOOR, // stairs?

    [0x22] = TILE_ATTR_WALK,
    [0x23] = TILE_ATTR_WALK | TILE_ATTR_NODE | TILE_ATTR_SWCH, // button
    [0x24] = TILE_ATTR_WALK | TILE_ATTR_NODE | TILE_ATTR_SWCH, // button

    [0x27] = 0,              // fence, or hammer peg?
    [0x28] = TILE_ATTR_LDGE, // up
    [0x29] = TILE_ATTR_LDGE, // down
    [0x2a] = TILE_ATTR_LDGE, // left
    [0x2b] = TILE_ATTR_LDGE, // right
    [0x2c] = TILE_ATTR_LDGE, // up left
    [0x2d] = TILE_ATTR_LDGE, // down left
    [0x2e] = TILE_ATTR_LDGE, // up right
    [0x2f] = TILE_ATTR_LDGE, // down right

    // Door transitions, unsure how to handle, see 0x8e & 0x8f
    [0x30] = 0,
    [0x31] = 0,
    [0x32] = 0,
    [0x33] = 0,
    [0x34] = 0,
    [0x35] = 0,
    [0x36] = 0,
    [0x37] = 0,
    [0x38] = 0,
    [0x39] = 0,

    [0x3d] = TILE_ATTR_WALK, // stairs?
    [0x3e] = TILE_ATTR_WALK, // stairs?

    [0x40] = TILE_ATTR_WALK,
    [0x48] = TILE_ATTR_WALK,
    [0x4B] = TILE_ATTR_WALK | TILE_ATTR_DOOR,

    [0x50] = TILE_ATTR_LFT0,
    [0x51] = TILE_ATTR_LFT0,
    [0x52] = TILE_ATTR_LFT1,
    [0x53] = TILE_ATTR_LFT2,
    [0x54] = 0, //TILE_ATTR_LFT0,
    [0x55] = TILE_ATTR_LFT1,
    [0x56] = TILE_ATTR_LFT2,
    [0x57] = TILE_ATTR_BONK,

    [0x58] = TILE_ATTR_NODE | TILE_ATTR_CHST,
    [0x59] = TILE_ATTR_NODE | TILE_ATTR_CHST,
    [0x5A] = TILE_ATTR_NODE | TILE_ATTR_CHST,
    [0x5B] = TILE_ATTR_NODE | TILE_ATTR_CHST,
    [0x5C] = TILE_ATTR_NODE | TILE_ATTR_CHST,
    [0x5D] = TILE_ATTR_NODE | TILE_ATTR_CHST,
    [0x5E] = TILE_ATTR_NODE | TILE_ATTR_DOOR, // stairs up
    [0x5F] = TILE_ATTR_NODE | TILE_ATTR_DOOR, // stairs down

    [0x70] = TILE_ATTR_NODE | TILE_ATTR_LFT0,
    [0x71] = TILE_ATTR_NODE | TILE_ATTR_LFT0,
    [0x72] = TILE_ATTR_NODE | TILE_ATTR_LFT0,
    [0x73] = TILE_ATTR_NODE | TILE_ATTR_LFT0,
    [0x74] = TILE_ATTR_NODE | TILE_ATTR_LFT0,
    [0x75] = TILE_ATTR_NODE | TILE_ATTR_LFT0,
    [0x76] = TILE_ATTR_NODE | TILE_ATTR_LFT0,
    [0x77] = TILE_ATTR_NODE | TILE_ATTR_LFT0,
    [0x78] = TILE_ATTR_NODE | TILE_ATTR_LFT0,
    [0x79] = TILE_ATTR_NODE | TILE_ATTR_LFT0,
    [0x7A] = TILE_ATTR_NODE | TILE_ATTR_LFT0,
    [0x7B] = TILE_ATTR_NODE | TILE_ATTR_LFT0,
    [0x7C] = TILE_ATTR_NODE | TILE_ATTR_LFT0,
    [0x7D] = TILE_ATTR_NODE | TILE_ATTR_LFT0,
    [0x7E] = TILE_ATTR_NODE | TILE_ATTR_LFT0,
    [0x7F] = TILE_ATTR_NODE | TILE_ATTR_LFT0,

    [0x80] = TILE_ATTR_WALK,
    [0x81] = TILE_ATTR_WALK,
    [0x82] = TILE_ATTR_WALK, // locked door?
    [0x83] = TILE_ATTR_WALK,
    [0x84] = TILE_ATTR_WALK,
    [0x85] = TILE_ATTR_WALK, // locked door?
    [0x86] = TILE_ATTR_WALK,
    [0x87] = TILE_ATTR_WALK,
    [0x88] = TILE_ATTR_WALK,
    [0x89] = TILE_ATTR_WALK,
    [0x8A] = TILE_ATTR_WALK,
    [0x8B] = TILE_ATTR_WALK,
    [0x8C] = TILE_ATTR_WALK,
    [0x8D] = TILE_ATTR_WALK,
    [0x8E] = TILE_ATTR_WALK,
    [0x8F] = TILE_ATTR_WALK,

    [0x90] = TILE_ATTR_WALK,
    [0x91] = TILE_ATTR_WALK,
    [0x92] = TILE_ATTR_WALK,
    [0x93] = TILE_ATTR_WALK,
    [0x94] = TILE_ATTR_WALK,
    [0x95] = TILE_ATTR_WALK,
    [0x96] = TILE_ATTR_WALK,
    [0x97] = TILE_ATTR_WALK,

    [0xA0] = TILE_ATTR_WALK,
    [0xA1] = TILE_ATTR_WALK,
    [0xA2] = TILE_ATTR_WALK,
    [0xA3] = TILE_ATTR_WALK,
    [0xA4] = TILE_ATTR_WALK,
    [0xA5] = TILE_ATTR_WALK,

    [0xF8] = TILE_ATTR_DOOR, // Locked door, 0x8000?
    // Fake/unknown
    [0xFF] = TILE_ATTR_WALK,
};

static const char * ap_tile_attr_name(uint8_t idx) {
    uint16_t attr = ap_tile_attrs[idx];
    static char sbuf[1024];
    char * buf = sbuf;
#define X(d) if (attr & CONCAT(TILE_ATTR_, d)) { \
    if (buf != sbuf) { *buf++ = '|'; } \
    buf += sprintf(buf, STRINGIFY(d)); \
    }
TILE_ATTR_LIST
#undef X
    return sbuf;
}

enum ap_link_state {
    LINK_STATE_GROUND = 0x0,
    LINK_STATE_FALLING_HOLE = 0x1,
    LINK_STATE_SWIMMING = 0x4,
    LINK_STATE_FALLING_LEDGE = 0x11,
};
/* Link State
    0x0 - ground state
    0x1 - falling into a hole
    0x2 - recoil from hitting wall / enemies 
    0x3 - spin attacking
    0x4 - swimming, or falling into water
    0x5 - Turtle Rock platforms
    0x6 recoil again (other movement)
    0x7 - hit by Agahnims bug zapper
    0x8 - using ether medallion
    0x9 - using bombos medallion
    0xA - using quake medallion
    0xB - ???
    0xC - ???
    0xD - ???
    0xE - ???
    0xF - ??? 
    0x10 - ???
    0x11 - falling off a ledge
    0x12 - used when coming out of a dash by pressing a direction other than the dash direction
    0x13 - hookshot
    0x14 - magic mirror
    0x15 - holding up an item
    0x16 - asleep in his bed
    0x17 - permabunny
    0x18 - stuck under a heavy rock
    0x19 - Receiving Ether Medallion
    0x1A - Receiving Bombos Medallion
    0x1B - Opening Desert Palace
    0x1C - temporary bunny
    0x1D - Rolling back from Gargoyle gate or PullForRupees object
    0x1E - The actual spin attack motion.
*/

#define SPRITE_ATTR_LIST \
    X(ENMY) /* Enemy, can hurt */ \
    X(BLOK) /* Blocked, do not try to walk through */ \
    X(SWCH) /* Button / state-changer */ \
    X(ITEM) /* Pick-up */ \
    X(TALK) /* Talk for progression item */ \
    X(FLLW) /* Thing that can follow you */ \

enum {
#define X(d) CONCAT(_SPRITE_ATTR_INDEX_, d),
SPRITE_ATTR_LIST
#undef X
};
enum {
#define X(d) CONCAT(SPRITE_ATTR_, d) = 1ul << CONCAT(_SPRITE_ATTR_INDEX_, d),
SPRITE_ATTR_LIST
#undef X
};
static const char * const ap_sprite_attr_names[] = {
#define X(d) [CONCAT(SPRITE_ATTR_, d)] = STRINGIFY(d),
SPRITE_ATTR_LIST
#undef X
};

static const uint16_t ap_sprite_attrs[256] = {
    [0x00] = SPRITE_ATTR_ENMY, // Raven
    [0x01] = SPRITE_ATTR_ENMY, // Vulture
    [0x02] = SPRITE_ATTR_ENMY, // Flying Stalfos Head
    [0x03] = 0, // Unused (Don't use it, the sprite's ASM pointer is invalid. It will certainly crash the game.)
    [0x04] = SPRITE_ATTR_SWCH, // Good Switch being pulled
    [0x05] = SPRITE_ATTR_SWCH, // Some other sort of switch being pulled, but from above?
    [0x06] = 0, // Bad Switch
    [0x07] = SPRITE_ATTR_SWCH, // switch again (facing up)
    [0x08] = SPRITE_ATTR_ENMY, // Octorock
    [0x09] = SPRITE_ATTR_ENMY, // Giant Moldorm (boss)
    [0x0A] = SPRITE_ATTR_ENMY, // Four Shooter Octorock
    [0x0B] = 0, // Chicken / Chicken Transformed into Lady
    [0x0C] = SPRITE_ATTR_ENMY, // Octorock
    [0x0D] = SPRITE_ATTR_ENMY, // Normal Buzzblob / Morphed Buzzblob (tra la la... look for Sahashrala)
    [0x0E] = SPRITE_ATTR_ENMY, // Plants with big mouths
    [0x0F] = SPRITE_ATTR_ENMY, // Octobaloon (Probably the thing that explodes into 10 others)

    [0x10] = SPRITE_ATTR_ENMY, // Small things from the exploder (exploder? wtf is that?)
    [0x11] = SPRITE_ATTR_ENMY, // One Eyed Giants (bomb throwers) aka Hinox
    [0x12] = SPRITE_ATTR_ENMY, // Moblin Spear Throwers
    [0x13] = SPRITE_ATTR_ENMY, // Helmasaur?
    [0x14] = SPRITE_ATTR_BLOK, // Gargoyles Domain Gate
    [0x15] = SPRITE_ATTR_ENMY, // Fire Faery
    [0x16] = SPRITE_ATTR_BLOK, // Sahashrala / Aginah, sage of the desert
    [0x17] = SPRITE_ATTR_ENMY, // Water Bubbles?
    [0x18] = SPRITE_ATTR_ENMY, // Moldorm
    [0x19] = SPRITE_ATTR_ENMY, // Poe
    [0x1A] = 0, // Dwarf, Mallet, and the shrapnel from it hitting
    [0x1B] = 0, // Arrow in wall?
    [0x1C] = SPRITE_ATTR_BLOK, // Moveable Statue
    [0x1D] = SPRITE_ATTR_BLOK, // Weathervane
    [0x1E] = SPRITE_ATTR_BLOK | SPRITE_ATTR_SWCH, // Crystal Switch
    [0x1F] = SPRITE_ATTR_BLOK, // Sick Kid with Bug Catching Net

    [0x20] = SPRITE_ATTR_ENMY, // Bomb Slugs
    [0x21] = SPRITE_ATTR_SWCH, // Push Switch (like in Swamp Palace)
    [0x22] = SPRITE_ATTR_ENMY, // Darkworld Snakebasket
    [0x23] = SPRITE_ATTR_SWCH, // Red Onoff
    [0x24] = SPRITE_ATTR_SWCH, // Blue Onoff
    [0x25] = SPRITE_ATTR_BLOK, // Tree you can talk to?
    [0x26] = SPRITE_ATTR_ENMY, // Charging Octopi?
    [0x27] = SPRITE_ATTR_ENMY, // Dead Rocks? (Gorons bleh)
    [0x28] = SPRITE_ATTR_BLOK, // Shrub Guy who talks about Triforce / Other storytellers
    [0x29] = SPRITE_ATTR_BLOK, // Blind Hideout attendant
    [0x2A] = SPRITE_ATTR_BLOK, // Sweeping Lady
    [0x2B] = SPRITE_ATTR_BLOK, // Bum under the bridge + smoke and other effects like the fire
    [0x2C] = SPRITE_ATTR_BLOK, // Lumberjack Bros. 
    [0x2D] = 0, // Telepathic stones?
    [0x2E] = SPRITE_ATTR_BLOK, // Flute Boys Notes
    [0x2F] = SPRITE_ATTR_BLOK, // Heart Piece Race guy and girl
    [0x30] = SPRITE_ATTR_BLOK, // Person? (HM name)
    [0x31] = SPRITE_ATTR_BLOK, // Fortune Teller / Dwarf swordsmith
    [0x32] = SPRITE_ATTR_BLOK, // ??? (something with a turning head) 
    [0x33] = 0, // Pull the wall for rupees
    [0x34] = SPRITE_ATTR_BLOK, // ScaredGirl2 (HM name)
    [0x35] = SPRITE_ATTR_BLOK, // Innkeeper
    [0x36] = SPRITE_ATTR_BLOK, // Witch / Cane of Byrna?
    [0x37] = 0, // Waterfall
    [0x38] = SPRITE_ATTR_BLOK, // Arrow Target (e.g. Big Eye in Dark Palace)
    [0x39] = SPRITE_ATTR_BLOK, // Middle Aged Guy in the desert
    [0x3A] = 0, // Magic Powder Bat /The Lightning Bolt the bat hurls at you.
    [0x3B] = 0, // Dash Item / such as Book of Mudora, keys
    [0x3C] = SPRITE_ATTR_BLOK, // Kid in village near the trough
    [0x3D] = SPRITE_ATTR_BLOK, // Signs? Chicken lady also showed up / Scared ladies outside houses.
    [0x3E] = SPRITE_ATTR_ENMY, // Rock Rupee Crabs
    [0x3F] = SPRITE_ATTR_ENMY, // Tutorial Soldiers from beginning of game
    
    [0x40] = SPRITE_ATTR_ENMY | SPRITE_ATTR_BLOK, // Hyrule Castle Barrier to Agahnims Tower
    [0x41] = SPRITE_ATTR_ENMY, // Soldier
    [0x42] = SPRITE_ATTR_ENMY, // Blue Soldier
    [0x43] = SPRITE_ATTR_ENMY, // Red Spear Soldier
    [0x44] = SPRITE_ATTR_ENMY, // Crazy Blue Killer Soldiers
    [0x45] = SPRITE_ATTR_ENMY, // Crazy Red Spear Soldiers (And green ones in the village)
    [0x46] = SPRITE_ATTR_ENMY, // Blue Archer Soldiers
    [0x47] = SPRITE_ATTR_ENMY, // Green Archer Soldiers (in the bushes)
    [0x48] = SPRITE_ATTR_ENMY, // Red Spear Soldiers (in special armor)
    [0x49] = SPRITE_ATTR_ENMY, // Red Spear Soldiers (in the bushes)
    [0x4A] = SPRITE_ATTR_ENMY, // Red Bomb Soldiers
    [0x4B] = SPRITE_ATTR_ENMY, // Green Soldier Recruits (the idiots)
    [0x4C] = SPRITE_ATTR_ENMY, // Sand Monsters
    [0x4D] = SPRITE_ATTR_ENMY, // Flailing Bunnies on the ground
    [0x4E] = SPRITE_ATTR_ENMY, // Snakebasket
    [0x4F] = SPRITE_ATTR_ENMY, // Blobs?
    
    [0x50] = SPRITE_ATTR_ENMY, // Metal Balls (in Eastern Palace)
    [0x51] = SPRITE_ATTR_ENMY, // Armos
    [0x52] = 0, // Giant Zora
    [0x53] = SPRITE_ATTR_ENMY, // Armos Knights Boss
    [0x54] = SPRITE_ATTR_ENMY, // Lanmolas boss
    [0x55] = SPRITE_ATTR_ENMY, // Zora / Fireballs (including the blue Agahnim fireballs)
    [0x56] = SPRITE_ATTR_ENMY, // Walking Zora
    [0x57] = SPRITE_ATTR_BLOK, // Desert Palace Barriers
    [0x58] = SPRITE_ATTR_ENMY, // Sandcrab
    [0x59] = SPRITE_ATTR_ENMY, // Birds (boids)
    [0x5A] = SPRITE_ATTR_ENMY, // Squirrels
    [0x5B] = SPRITE_ATTR_ENMY, // Energy Balls (that crawl along the wall)
    [0x5C] = SPRITE_ATTR_ENMY, // Wall crawling fireballs
    [0x5D] = SPRITE_ATTR_ENMY, // Roller (vertical moving)
    [0x5E] = SPRITE_ATTR_ENMY, // Roller (vertical moving)
    [0x5F] = SPRITE_ATTR_ENMY, // Roller
    
    [0x60] = SPRITE_ATTR_ENMY, // Roller (horizontal moving)
    [0x61] = SPRITE_ATTR_BLOK, // Statue Sentry
    [0x62] = SPRITE_ATTR_BLOK, // Master Sword plus pendants and beams of light
    [0x63] = 0, // Sand Lion Pit
    [0x64] = SPRITE_ATTR_ENMY, // Sand Lion
    [0x65] = 0, // Shooting Gallery guy
    [0x66] = SPRITE_ATTR_BLOK, // Moving cannon ball shooters
    [0x67] = SPRITE_ATTR_BLOK, // Moving cannon ball shooters
    [0x68] = SPRITE_ATTR_BLOK, // Moving cannon ball shooters
    [0x69] = SPRITE_ATTR_BLOK, // Moving cannon ball shooters 
    [0x6A] = SPRITE_ATTR_ENMY, // Ball N' Chain Trooper
    [0x6B] = SPRITE_ATTR_ENMY, // Cannon Ball Shooting Soldier (unused in original = WTF?)
    [0x6C] = 0, // Warp Vortex created by Magic Mirror
    [0x6D] = SPRITE_ATTR_ENMY, // Mouse
    [0x6E] = SPRITE_ATTR_ENMY, // Snakes (forgot the Zelda 1 name)
    [0x6F] = SPRITE_ATTR_ENMY, // Bats / Also one eyed bats
    
    [0x70] = SPRITE_ATTR_ENMY, // Splitting Fireballs from Helmasaur King
    [0x71] = SPRITE_ATTR_SWCH, // Leever
    [0x72] = 0, // Activator for the ponds (where you throw in items)
    [0x73] = SPRITE_ATTR_TALK, // Links Uncle / Sage / Barrier that opens in the sanctuary
    [0x74] = 0, // Red Hat Boy who runs from you
    [0x75] = SPRITE_ATTR_TALK, // Bottle Vendor
    [0x76] = SPRITE_ATTR_FLLW | SPRITE_ATTR_BLOK, // Princess Zelda
    [0x77] = SPRITE_ATTR_ENMY, // Also Fire Faeries (seems like a different variety)
    [0x78] = SPRITE_ATTR_BLOK, // Village Elder
    [0x79] = 0, // Good bee / normal bee
    [0x7A] = SPRITE_ATTR_ENMY, // Agahnim
    [0x7B] = SPRITE_ATTR_ENMY, // Agahnim energy blasts (not the duds)
    [0x7C] = 0, // Boos? As in the Boos from SM3/SMW? Thats crazy talk!
    [0x7D] = 0, // 32*32 Pixel Yellow Spike Traps
    [0x7E] = SPRITE_ATTR_ENMY, // Swinging Fireball Chains
    [0x7F] = SPRITE_ATTR_ENMY, // Swinging Fireball Chains
    
    [0x80] = SPRITE_ATTR_ENMY, // Wandering Fireball Chains
    [0x81] = SPRITE_ATTR_ENMY, // Waterhoppers
    [0x82] = SPRITE_ATTR_ENMY, // Swirling Fire Faeries (Eastern Palace)
    [0x83] = SPRITE_ATTR_ENMY, // Rocklops
    [0x84] = SPRITE_ATTR_ENMY, // Red Rocklops
    [0x85] = SPRITE_ATTR_ENMY, // Yellow Stalfos (drops to the ground, dislodges head)
    [0x86] = SPRITE_ATTR_ENMY, // Fire Breathing Dinos?
    [0x87] = SPRITE_ATTR_ENMY, // Flames
    [0x88] = SPRITE_ATTR_ENMY, // Mothula
    [0x89] = SPRITE_ATTR_ENMY, // Mothulas beam
    [0x8A] = SPRITE_ATTR_ENMY, // Key holes? Spikes that move
    [0x8B] = SPRITE_ATTR_ENMY, // Gibdos
    [0x8C] = SPRITE_ATTR_ENMY, // Arghuss
    [0x8D] = SPRITE_ATTR_ENMY, // Arghuss spawn
    [0x8E] = SPRITE_ATTR_ENMY, // Chair Turtles you kill with hammers
    [0x8F] = SPRITE_ATTR_ENMY, // Blobs / Crazy Blobs via Magic powder or Quake Medallion
    
    [0x90] = SPRITE_ATTR_ENMY, // Grabber things?
    [0x91] = SPRITE_ATTR_ENMY, // Stalfos Knight
    [0x92] = SPRITE_ATTR_ENMY, // Helmasaur King
    [0x93] = SPRITE_ATTR_ENMY, // Bungie / Red Orb? (according to HM)
    [0x94] = SPRITE_ATTR_ENMY, // Swimmers
    [0x95] = SPRITE_ATTR_ENMY, // Eye laser
    [0x96] = SPRITE_ATTR_ENMY, // Eye laser
    [0x97] = SPRITE_ATTR_ENMY, // Eye laser
    [0x98] = SPRITE_ATTR_ENMY, // Eye laser
    [0x99] = SPRITE_ATTR_ENMY, // Penguin
    [0x9A] = SPRITE_ATTR_ENMY, // Moving Water Bubble (only in Swamp Palace)
    [0x9B] = SPRITE_ATTR_ENMY, // Wizzrobes
    [0x9C] = SPRITE_ATTR_ENMY, // Black sperm looking things
    [0x9D] = SPRITE_ATTR_ENMY, // Black sperm looking things
    [0x9E] = 0, // The ostrich animal w/ the flute boy?
    [0x9F] = SPRITE_ATTR_ITEM, // Flute
    
    [0xA0] = 0, // Birds w/ the flute boy?
    [0xA1] = SPRITE_ATTR_ENMY, // Ice man
    [0xA2] = SPRITE_ATTR_ENMY, // Kholdstare
    [0xA3] = SPRITE_ATTR_ENMY, // Another part of Kholdstare
    [0xA4] = SPRITE_ATTR_ENMY, // Ice balls from above
    [0xA5] = SPRITE_ATTR_ENMY, // Blue Horse Knight, and Lynel Fireballs
    [0xA6] = SPRITE_ATTR_ENMY, // Red Horse Knight
    [0xA7] = SPRITE_ATTR_ENMY, // Red Stalfos Skeleton
    [0xA8] = SPRITE_ATTR_ENMY, // Bomber Flying Creatures from Darkworld
    [0xA9] = SPRITE_ATTR_ENMY, // Bomber Flying Creatures from Darkworld
    [0xAA] = SPRITE_ATTR_ENMY, // Like Like (O_o yikes)
    [0xAB] = 0, // Maiden (as in, the maidens in the crystals after you beat a boss)
    [0xAC] = 0, // Apples
    [0xAD] = SPRITE_ATTR_FLLW, // Old Man on the Mountain
    [0xAE] = 0, // Down Pipe
    [0xAF] = 0, // Up Pipe
    
    [0xB0] = 0, // Right Pipe
    [0xB1] = 0, // Left Pipe
    [0xB2] = 0, // Good bee again?
    [0xB3] = 0, // Hylian Inscription (near Desert Palace). Also near Master Sword
    [0xB4] = SPRITE_ATTR_FLLW, // Thiefs chest (not the one that follows you, the one that you grab from the DW smithy house)
    [0xB5] = SPRITE_ATTR_BLOK | SPRITE_ATTR_TALK, // Bomb Salesman (elephant looking guy)
    [0xB6] = SPRITE_ATTR_FLLW, // Kiki the monkey?
    [0xB7] = SPRITE_ATTR_FLLW, // Maiden following you in Blind Dungeon
    [0xB8] = 0, // Monologue Testing Sprite (Debug Artifact)
    [0xB9] = 0, // Feuding Friends on Death Mountain
    [0xBA] = 0, // Whirlpool
    // subtype2: 0x00 salesman; 0x0c bombs; 0x0a one heart; 0x07 red potion
    [0xBB] = SPRITE_ATTR_BLOK | SPRITE_ATTR_TALK, // Salesman / chestgame guy / 300 rupee giver guy / Chest game thief / Item for sale
    [0xBC] = SPRITE_ATTR_BLOK, // Drunk in the inn
    [0xBD] = SPRITE_ATTR_ENMY, // Vitreous (the large eyeball)
    [0xBE] = SPRITE_ATTR_ENMY, // Vitreous' smaller eyeballs
    [0xBF] = SPRITE_ATTR_ENMY, // Vitreous' lightning blast
    
    [0xC0] = 0, // Monster in Lake of Ill Omen / Quake Medallion
    [0xC1] = SPRITE_ATTR_ENMY, // Agahnim teleporting Zelda to dark world
    [0xC2] = SPRITE_ATTR_ENMY, // Boulders
    [0xC3] = SPRITE_ATTR_ENMY, // Symbions 2 (vulnerable part)
    [0xC4] = SPRITE_ATTR_ENMY, // Thief
    [0xC5] = SPRITE_ATTR_ENMY, // Evil Fireball Spitters (THE FACES!!!)
    [0xC6] = SPRITE_ATTR_ENMY, // Four Way Fireball Spitters (spit when you use your sword)
    [0xC7] = SPRITE_ATTR_ENMY, // Fuzzy Stack
    [0xC8] = SPRITE_ATTR_TALK, // Big Healing Faeries / Faerie Dust
    [0xC9] = SPRITE_ATTR_ENMY, // Ganons Firebat (HM also says Tektite?)
    [0xCA] = SPRITE_ATTR_ENMY, // Chain Chomp
    [0xCB] = SPRITE_ATTR_ENMY, // Trinexx
    [0xCC] = SPRITE_ATTR_ENMY, // Another Part of Trinexx
    [0xCD] = SPRITE_ATTR_ENMY, // Another Part of Trinexx (again)
    [0xCE] = SPRITE_ATTR_ENMY, // Blind the Thief
    [0xCF] = SPRITE_ATTR_ENMY, // Swamp Worms (from Swamp of Evil)
    
    [0xD0] = SPRITE_ATTR_ENMY, // Lynel (centaur like creature)
    [0xD1] = SPRITE_ATTR_ENMY, // A yellow hunter
    [0xD2] = SPRITE_ATTR_ENMY, // Flopping fish
    [0xD3] = SPRITE_ATTR_ENMY, // Animated Skulls Creatures
    [0xD4] = SPRITE_ATTR_ENMY, // Landmines
    [0xD5] = SPRITE_ATTR_BLOK | SPRITE_ATTR_TALK, // Digging Game Proprietor
    [0xD6] = SPRITE_ATTR_ENMY, // Ganon! OMG
    [0xD7] = SPRITE_ATTR_ENMY, // Copy of Ganon, except invincible?
    [0xD8] = SPRITE_ATTR_ITEM, // Heart refill
    [0xD9] = SPRITE_ATTR_ITEM, // Green Rupee
    [0xDA] = SPRITE_ATTR_ITEM, // Blue Rupee
    [0xDB] = SPRITE_ATTR_ITEM, // Red Rupee
    [0xDC] = SPRITE_ATTR_ITEM, // Bomb Refill (1)
    [0xDD] = SPRITE_ATTR_ITEM, // Bomb Refill (4)
    [0xDE] = SPRITE_ATTR_ITEM, // Bomb Refill (8)
    [0xDF] = SPRITE_ATTR_ITEM, // Small Magic Refill
    
    [0xE0] = SPRITE_ATTR_ITEM, // Full Magic Refill
    [0xE1] = SPRITE_ATTR_ITEM, // Arrow Refill (5)
    [0xE2] = SPRITE_ATTR_ITEM, // Arrow Refill (10)
    [0xE3] = SPRITE_ATTR_ITEM, // Faerie
    [0xE4] = SPRITE_ATTR_ITEM, // Key
    [0xE5] = SPRITE_ATTR_ITEM, // Big Key
    [0xE6] = SPRITE_ATTR_ITEM, // Red Shield (after dropped by pickit)
    [0xE7] = SPRITE_ATTR_ITEM, // Mushroom
    [0xE8] = 0, // Fake Master Sword
    [0xE9] = 0, // Magic Shop dude / His items, including the magic powder
    [0xEA] = SPRITE_ATTR_ITEM, // Full Heart Container
    [0xEB] = SPRITE_ATTR_ITEM, // Quarter Heart Container
    [0xEC] = SPRITE_ATTR_BLOK, // Bushes
    [0xED] = 0, // Cane of Somaria Platform
    [0xEE] = SPRITE_ATTR_BLOK, // Movable Mantle (in Hyrule Castle)
    [0xEF] = 0, // Cane of Somaria Platform (same as 0xED but this index is not used)
    
    [0xF0] = 0, // Cane of Somaria Platform (same as 0xED but this index is not used)
    [0xF1] = 0, // Cane of Somaria Platform (same as 0xED but this index is not used)
    [0xF2] = SPRITE_ATTR_BLOK, // Medallion Tablet
};

static const char * ap_sprite_attr_name(uint8_t idx) {
    uint16_t attr = ap_sprite_attrs[idx];
    static char sbuf[1024];
    char * buf = sbuf;
#define X(d) if (attr & CONCAT(SPRITE_ATTR_, d)) { \
    if (buf != sbuf) { *buf++ = '|'; } \
    buf += sprintf(buf, STRINGIFY(d)); \
    }
SPRITE_ATTR_LIST
#undef X
    return sbuf;
}

// module_index
// 0x00 Nintendo presents screen + Title screen
// 0x01 Player select screen
// 0x02 Copy Player Mode
// 0x03 Erase Player Mode
// 0x04 Register your name screen
// 0x05 Loading Game Mode
// 0x06 Pre House/Dungeon Mode
// 0x07 In house/dungeon
// 0x08 Pre Overworld Mode
// 0x09 In OW
// 0x0A Pre Overworld Mode (special overworld)
// 0x0B Overworld Mode (special overworld)
// 0x0C Unused?
// 0x0D Blank Screen
// 0x0E Text Mode/Item Screen/Map
// 0x0F Closing Spotlight
// 0x10 Opening Spotlight
// 0x11 Happens when you fall into a hole from the OW.
// 0x12 Death Mode
// 0x13 Boss Victory Mode (refills stats)
// 0x14 Cutscene in title screen (history sequence)
// 0x15 Module for Magic Mirror
// 0x16 Module for refilling stats after boss.
// 0x17 Restart mode (save and quit)
// 0x18 Ganon exits from Agahnim's body. Chase Mode.
// 0x19 Triforce Room scene
// 0x1A End sequence
// 0x1B Screen to select where to start from (House, sanctuary, etc.)
