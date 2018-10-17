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
    X(frame_counter,        uint8_t,   0x7E001A)   \
    X(in_building,          uint8_t,   0x7E001B)   \
    X(area,                 uint8_t,   0x7E008A)   \
    X(link_x,               uint16_t,  0x7E0022)   \
    X(link_y,               uint16_t,  0x7E0020)   \
    X(link_dx,              uint8_t,   0x7E0031)   \
    X(link_dy,              uint8_t,   0x7E0030)   \
    X(link_hold,            uint16_t,  0x7E0308)   \
    X(link_direction,       uint8_t,   0x7E002F)   \
    X(dungeon_room,         uint16_t,  0x7E00A0)   \
    X(menu_part,            uint8_t,   0x7E00C8)   \
    X(sprite_y_lo,          uint8_t,   0x7E0D00)   \
    X(sprite_x_lo,          uint8_t,   0x7E0D10)   \
    X(sprite_y_hi,          uint8_t,   0x7E0D20)   \
    X(sprite_x_hi,          uint8_t,   0x7E0D30)   \
    X(sprite_type,          uint8_t,   0x7E0E20)   \
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
    X(carrying_bit7,        uint8_t,   0x7E0308)   \
    X(ignore_sprites,       uint8_t,   0x7E037B)   \

extern struct ap_ram {
#define X(name, type, offset) const type * name;
AP_RAM_LIST
#undef X
} ap_ram;

extern struct ap_snes9x * ap_emu;

#define TILE_ATTR_WALK 0x0001 // Walkable
#define TILE_ATTR_SWIM 0x0002 // Swim-able
#define TILE_ATTR_DOOR 0x0004 // Door or other transition
#define TILE_ATTR_ITEM 0x0008 // Item (chest, or something under a pot)
#define TILE_ATTR_LFT0 0x0010 // Can lift with no power ups
#define TILE_ATTR_LFT1 0x0020 // Can lift with power glove
#define TILE_ATTR_LFT2 0x0040 // Can lift with titans mitts
#define TILE_ATTR_LDGE 0x0080 // Ledge
#define TILE_ATTR_CHST 0x0100 // Chest
#define TILE_ATTR_BONK 0x0200 // Bonk rocks

static const uint16_t ap_tile_attrs[256] = {
    [0x00] = TILE_ATTR_WALK,
    [0x01] = 0,
    [0x02] = 0,

    [0x08] = TILE_ATTR_SWIM,
    [0x09] = TILE_ATTR_WALK,

    [0x22] = TILE_ATTR_WALK,

    [0x27] = 0,              // fence
    [0x28] = TILE_ATTR_LDGE, // up
    [0x29] = TILE_ATTR_LDGE, // down
    [0x2a] = TILE_ATTR_LDGE, // left
    [0x2b] = TILE_ATTR_LDGE, // right
    [0x2c] = TILE_ATTR_LDGE, // up left
    [0x2d] = TILE_ATTR_LDGE, // down left
    [0x2e] = TILE_ATTR_LDGE, // up right
    [0x2f] = TILE_ATTR_LDGE, // down right

    [0x30] = TILE_ATTR_DOOR,
    [0x31] = TILE_ATTR_DOOR,
    [0x32] = TILE_ATTR_DOOR,
    [0x33] = TILE_ATTR_DOOR,
    [0x34] = TILE_ATTR_DOOR,
    [0x35] = TILE_ATTR_DOOR,
    [0x36] = TILE_ATTR_DOOR,
    [0x37] = TILE_ATTR_DOOR,
    [0x38] = TILE_ATTR_DOOR,
    [0x39] = TILE_ATTR_DOOR,

    [0x40] = TILE_ATTR_WALK,
    [0x48] = TILE_ATTR_WALK,
    [0x4B] = TILE_ATTR_DOOR,

    [0x50] = TILE_ATTR_LFT0,
    [0x51] = TILE_ATTR_LFT0,
    [0x52] = TILE_ATTR_LFT1,
    [0x53] = TILE_ATTR_LFT2,
    [0x54] = TILE_ATTR_LFT0,
    [0x55] = TILE_ATTR_LFT1,
    [0x56] = TILE_ATTR_LFT2,
    [0x57] = TILE_ATTR_BONK,

    [0x58] = TILE_ATTR_ITEM | TILE_ATTR_CHST,
    [0x59] = TILE_ATTR_ITEM | TILE_ATTR_CHST,
    [0x5A] = TILE_ATTR_ITEM | TILE_ATTR_CHST,
    [0x5B] = TILE_ATTR_ITEM | TILE_ATTR_CHST,
    [0x5C] = TILE_ATTR_ITEM | TILE_ATTR_CHST,
    [0x5D] = TILE_ATTR_ITEM | TILE_ATTR_CHST,

    [0x70] = TILE_ATTR_ITEM | TILE_ATTR_LFT0,
    [0x71] = TILE_ATTR_ITEM | TILE_ATTR_LFT0,
    [0x72] = TILE_ATTR_ITEM | TILE_ATTR_LFT0,
    [0x73] = TILE_ATTR_ITEM | TILE_ATTR_LFT0,
    [0x74] = TILE_ATTR_ITEM | TILE_ATTR_LFT0,
    [0x75] = TILE_ATTR_ITEM | TILE_ATTR_LFT0,
    [0x76] = TILE_ATTR_ITEM | TILE_ATTR_LFT0,
    [0x77] = TILE_ATTR_ITEM | TILE_ATTR_LFT0,
    [0x78] = TILE_ATTR_ITEM | TILE_ATTR_LFT0,
    [0x79] = TILE_ATTR_ITEM | TILE_ATTR_LFT0,
    [0x7A] = TILE_ATTR_ITEM | TILE_ATTR_LFT0,
    [0x7B] = TILE_ATTR_ITEM | TILE_ATTR_LFT0,
    [0x7C] = TILE_ATTR_ITEM | TILE_ATTR_LFT0,
    [0x7D] = TILE_ATTR_ITEM | TILE_ATTR_LFT0,
    [0x7E] = TILE_ATTR_ITEM | TILE_ATTR_LFT0,
    [0x7F] = TILE_ATTR_ITEM | TILE_ATTR_LFT0,

    [0x80] = TILE_ATTR_DOOR,
    [0x8E] = TILE_ATTR_DOOR,

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

    // Fake/unknown
    [0xFF] = TILE_ATTR_WALK,
};
