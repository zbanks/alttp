#pragma once
#include "ap_macro.h"
#include "ap_math.h"
#include "alttp_public.h"

#define SNES_TR_MASK        ((uint16_t) (1ul <<  4))
#define SNES_TL_MASK        ((uint16_t) (1ul <<  5))
#define SNES_X_MASK         ((uint16_t) (1ul <<  6))
#define SNES_A_MASK         ((uint16_t) (1ul <<  7))
#define SNES_RIGHT_MASK     ((uint16_t) (1ul <<  8))
#define SNES_LEFT_MASK      ((uint16_t) (1ul <<  9))
#define SNES_DOWN_MASK      ((uint16_t) (1ul << 10))
#define SNES_UP_MASK        ((uint16_t) (1ul << 11))
#define SNES_START_MASK     ((uint16_t) (1ul << 12))
#define SNES_SELECT_MASK    ((uint16_t) (1ul << 13))
#define SNES_Y_MASK         ((uint16_t) (1ul << 14))
#define SNES_B_MASK         ((uint16_t) (1ul << 15))

#define SNES_MASK(key) CONCAT(CONCAT(SNES_, key), _MASK)

#define JOYPAD_TEST(key) ((*joypad) & SNES_MASK(key))
#define JOYPAD_SET(key) ((*joypad) |= SNES_MASK(key))
#define JOYPAD_CLEAR(key) ((*joypad) &= (uint16_t) ~SNES_MASK(key))
#define JOYPAD_MASH(key, rate) ({ if(*ap_ram.frame_counter & (rate)) { JOYPAD_SET(key); } else { JOYPAD_CLEAR(key); } })
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

bool ap_manual_mode;

#define AP_RAM_LIST \
    X(module_index,         uint8_t,   0x7E0010)   \
    X(submodule_index,      uint8_t,   0x7E0011)   \
    X(sub_submodule_index,  uint8_t,   0x7E00B0)   \
    X(frame_counter,        uint8_t,   0x7E001A)   \
    X(in_building,          uint8_t,   0x7E001B)   \
    X(main_screen_bitmask,  uint8_t,   0x7E001C)   \
    X(sub_screen_bitmask,   uint8_t,   0x7E001D)   \
    X(area,                 uint8_t,   0x7E008A)   \
    X(link_x,               uint16_t,  0x7E0022)   \
    X(link_y,               uint16_t,  0x7E0020)   \
    X(link_dx,              int8_t,    0x7E0031)   \
    X(link_dy,              int8_t,    0x7E0030)   \
    X(link_hold,            uint16_t,  0x7E0308)   \
    X(link_direction,       uint8_t,   0x7E002F)   \
    X(link_swordstate,      uint8_t,   0x7E003A)   \
    X(link_falling,         uint8_t,   0x7E005B)   \
    X(link_state,           uint8_t,   0x7E005D)   \
    X(link_lower_level,     uint8_t,   0x7E00EE)   \
    X(link_on_switch,       uint16_t,  0x7E0430)   \
    X(overworld_index,      uint16_t,  0x7E008A)   \
    X(overworld_dark,       uint8_t,   0x7E0FFF)   \
    X(dungeon_room,         uint16_t,  0x7E00A0)   \
    X(dungeon_tags,         uint16_t,  0x7E00AE)   \
    X(menu_part,            uint8_t,   0x7E00C8)   \
    X(screen_transition,    uint8_t,   0x7E0126)   \
    X(current_item,         uint8_t,   0x7E0202)   \
    X(recving_item,         uint8_t,   0x7E02D8)   \
    X(room_state,           uint16_t,  0x7E0400)   \
    X(room_chest_state,     uint8_t,   0x7E0403)   \
    X(room_layout,          uint8_t,   0x7E040E)   \
    X(room_trap_doors,      uint16_t,  0x7E0468)   \
    X(room_chest_index,     uint16_t,  0x7E0496)   \
    X(room_keyblock_index,  uint16_t,  0x7E0498)   \
    X(crystal_timer,        uint8_t,   0x7E04C2)   \
    X(room_chest_tilemaps,  uint16_t,  0x7E06E0)   \
    X(dngn_open_doors,      uint16_t,  0x7E068C)   \
    X(sprite_drop,          uint8_t,   0x7E0CBA)   \
    X(sprite_y_lo,          uint8_t,   0x7E0D00)   \
    X(sprite_x_lo,          uint8_t,   0x7E0D10)   \
    X(sprite_y_hi,          uint8_t,   0x7E0D20)   \
    X(sprite_x_hi,          uint8_t,   0x7E0D30)   \
    X(sprite_spawned,       uint8_t,   0x7E0D80)   \
    X(sprite_state,         uint8_t,   0x7E0DD0)   \
    X(sprite_type,          uint8_t,   0x7E0E20)   \
    X(sprite_subtype1,      uint8_t,   0x7E0E30)   \
    X(sprite_hp,            int8_t,    0x7E0E50)   \
    X(sprite_interaction,   uint8_t,   0x7E0E60)   \
    X(sprite_subtype2,      uint8_t,   0x7E0E80)   \
    X(sprite_lower_level,   uint8_t,   0x7E0F20)   \
    X(sprite_hitbox_idx,    uint8_t,   0x7E0F60)   \
    X(overlord_types,       uint8_t,   0x7E0B00)   \
    X(overlord_timers,      uint8_t,   0x7E0B28)   \
    X(ancillia_bf0,         uint8_t,   0x7E0BF0)   \
    X(ancillia_y_lo,        uint8_t,   0x7E0BFA)   \
    X(ancillia_x_lo,        uint8_t,   0x7E0C04)   \
    X(ancillia_y_hi,        uint8_t,   0x7E0C0E)   \
    X(ancillia_x_hi,        uint8_t,   0x7E0C18)   \
    X(ancillia_y_vel,       int8_t,    0x7E0C22)   \
    X(ancillia_x_vel,       int8_t,    0x7E0C2C)   \
    X(ancillia_y_sub,       uint8_t,   0x7E0C36)   \
    X(ancillia_x_sub,       uint8_t,   0x7E0C40)   \
    X(ancillia_type,        uint8_t,   0x7E0C4A)   \
    X(ancillia_lower_level, uint8_t,   0x7E0C7C)   \
    X(hitbox_x_lo,          uint8_t,   0x06F735)   \
    X(hitbox_x_hi,          uint8_t,   0x06F755)   \
    X(hitbox_width,         uint8_t,   0x06F775)   \
    X(hitbox_y_lo,          uint8_t,   0x06F795)   \
    X(hitbox_y_hi,          uint8_t,   0x06F7B5)   \
    X(hitbox_height,        uint8_t,   0x06F7D5)   \
    X(block_data,           uint32_t,  0x7EF940)   \
    X(map_area,             uint16_t,  0x7E040A)   \
    X(dungeon_id,           uint16_t,  0x7E040C)   \
    X(map_y_offset,         uint16_t,  0x7E0708)   \
    X(map_y_mask,           uint16_t,  0x7E070A)   \
    X(map_x_offset,         uint16_t,  0x7E070C)   \
    X(map_x_mask,           uint16_t,  0x7E070E)   \
    X(map_area_size,        uint16_t,  0x7E0712)   \
    X(over_map16,           uint16_t,  0x7E2000)   \
    X(dngn_bg1_map8,        uint16_t,  0x7E2000)   \
    X(dngn_bg2_map8,        uint16_t,  0x7E4000)   \
 /* X(map16_to_map8,        uint64_t,  0x078000) */\
 /* X(chr_to_tattr,         uint64_t,  0x078000) */\
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
    X(over_overlay_map16s,  uint16_t,  0x02EB29)   \
    X(touching_chest,       uint8_t,   0x7E02E5)   \
    X(item_recv_method,     uint8_t,   0x7E02E9)   \
    X(push_dir_bitmask,     uint8_t,   0x7E0026)   \
    X(push_timer,           uint8_t,   0x7E0371)   \
    X(carrying_bit7,        uint8_t,   0x7E0308)   \
    X(ignore_sprites,       uint8_t,   0x7E037B)   \
    X(sram_room_state,      uint16_t,  0x7EF000)   \
    X(sram_overworld_state, uint8_t,   0x7EF280)   \
    X(sram_goal_rupees,     uint16_t,  0x7EF360)   \
    X(sram_actual_rupees,   uint16_t,  0x7EF362)   \
    X(sram_dungeon_bigkeys, uint16_t,  0x7EF366)   \
    X(sram_pendants,        uint8_t,   0x7EF374)   \
    X(sram_crystals,        uint8_t,   0x7EF37A)   \
    X(sram_dungeon_keys,    uint8_t,   0x7EF37C)   \
    X(sram_progress1,       uint8_t,   0x7EF3C5)   \
    X(sram_progress2,       uint8_t,   0x7EF3C6)   \
    X(sram_progress3,       uint8_t,   0x7EF3C9)   \
    X(sram_tagalong,        uint8_t,   0x7EF3CC)   \
    X(inventory_base,       uint8_t,   0x7EF33F)   \
    X(inventory_bombs,      uint8_t,   0x7EF343)   \
    X(inventory_hammer,     uint8_t,   0x7EF34B)   \
    X(inventory_gloves,     uint8_t,   0x7EF354)   \
    X(inventory_sword,      uint8_t,   0x7EF359)   \
    X(inventory_quiver,     uint8_t,   0x7EF377)   \
    X(dungeon_bigkeys,      uint16_t,  0x7EF366)   \
    X(dungeon_keys,         uint8_t,   0x7EF37C)   \
    X(dungeon_current_keys, uint8_t,   0x7EF36F)   \
    X(health_current,       uint8_t,   0x7EF36D)   \
    X(health_capacity,      uint8_t,   0x7EF36C)   \
    X(dungeon_door_types,   uint16_t,  0x7E1980)   \
    X(dungeon_door_tilemaps,uint16_t,  0x7E19A0)   \
    X(dungeon_door_dirs,    uint16_t,  0x7E19C0)   \
    X(dungeon_chests,       uint8_t,   0x01E96C)   \
    X(dngn_stairs_0,        uint16_t,  0x000438)   \
    X(dngn_stairs_1,        uint16_t,  0x00047E)   \
    X(dngn_stairs_2,        uint16_t,  0x000482)   \
    X(dngn_stairs_3,        uint16_t,  0x0004A2)   \
    X(dngn_stairs_4,        uint16_t,  0x0004A4)   \
    X(dngn_stairs_5,        uint16_t,  0x00043A)   \
    X(dngn_stairs_6,        uint16_t,  0x000480)   \
    X(dngn_stairs_7,        uint16_t,  0x000484)   \
    X(dngn_stairs_8,        uint16_t,  0x0004A6)   \
    X(dngn_stairs_9,        uint16_t,  0x0004A8)   \
    X(special_tilemaps,     uint16_t,  0x0006B0)   \

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
    X(NODE) /* Important goal node (chest, or something under a pot, stairs) */ \
    X(LFT0) /* Can lift with no power ups */ \
    X(LFT1) /* Can lift with power glove */ \
    X(LFT2) /* Can lift with titans mitts */ \
    X(LDGE) /* Ledge */ \
    X(CHST) /* Chest */ \
    X(BONK) /* Bonk rocks */ \
    X(SWCH) /* Floor button */ \
    X(STRS) /* Dungeon Stairs */ \
    X(MERG) /* Merge with similar tiles */ \
    X(EDGE) /* Pits have weird collision; only check top edge */ \
    X(HMMR) /* Hammer pegs */ \

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
extern const char * const ap_tile_attr_names[];
const char * ap_tile_attr_name(uint8_t idx);

static const uint16_t ap_tile_attrs[256] = {
    [0x00] = TILE_ATTR_WALK,
    [0x01] = 0,
    [0x02] = 0,

    [0x08] = TILE_ATTR_SWIM,
    [0x09] = TILE_ATTR_WALK,
    [0x10] = 0, // edge of ledge?
    [0x1c] = TILE_ATTR_WALK, // open below?
    // XXX how do these work; how are they different than 0x3X
    [0x1d] = TILE_ATTR_WALK, // stairs?
    [0x1e] = TILE_ATTR_NODE | TILE_ATTR_STRS, // in-room stairs?
    [0x1f] = TILE_ATTR_NODE | TILE_ATTR_STRS, // in-room stairs?
    [0x20] = TILE_ATTR_EDGE, // Pit

    [0x22] = TILE_ATTR_WALK,
    [0x23] = TILE_ATTR_WALK | TILE_ATTR_NODE | TILE_ATTR_SWCH, // button 0x8000
    [0x24] = TILE_ATTR_WALK | TILE_ATTR_NODE | TILE_ATTR_SWCH, // button 0x4000
    [0x25] = TILE_ATTR_WALK | TILE_ATTR_NODE | TILE_ATTR_SWCH, // button XXX WILD GUESS
    [0x26] = TILE_ATTR_NODE | TILE_ATTR_DOOR, // south stairs up
    //[0x26] = TILE_ATTR_WALK | TILE_ATTR_NODE | TILE_ATTR_SWCH, // button XXX WILD GUESS

    [0x27] = TILE_ATTR_HMMR, // hammer peg (was also fence; remapped to 0x01)
    [0x28] = TILE_ATTR_LDGE, // up
    [0x29] = TILE_ATTR_LDGE, // down
    [0x2a] = TILE_ATTR_LDGE, // left
    [0x2b] = TILE_ATTR_LDGE, // right
    [0x2c] = TILE_ATTR_LDGE, // up left
    [0x2d] = TILE_ATTR_LDGE, // down left
    [0x2e] = TILE_ATTR_LDGE, // up right
    [0x2f] = TILE_ATTR_LDGE, // down right

    // Door transitions, unsure how to handle, see 0x8e & 0x8f
    [0x30] = TILE_ATTR_NODE | TILE_ATTR_DOOR | TILE_ATTR_MERG,
    [0x31] = TILE_ATTR_NODE | TILE_ATTR_DOOR | TILE_ATTR_MERG,
    [0x32] = TILE_ATTR_NODE | TILE_ATTR_DOOR | TILE_ATTR_MERG,
    [0x33] = TILE_ATTR_NODE | TILE_ATTR_DOOR | TILE_ATTR_MERG,
    [0x34] = TILE_ATTR_NODE | TILE_ATTR_DOOR | TILE_ATTR_MERG,
    [0x35] = TILE_ATTR_NODE | TILE_ATTR_DOOR | TILE_ATTR_MERG,
    [0x36] = TILE_ATTR_NODE | TILE_ATTR_DOOR | TILE_ATTR_MERG,
    [0x37] = TILE_ATTR_NODE | TILE_ATTR_DOOR | TILE_ATTR_MERG,
    [0x38] = TILE_ATTR_NODE | TILE_ATTR_DOOR | TILE_ATTR_MERG,
    [0x39] = TILE_ATTR_NODE | TILE_ATTR_DOOR | TILE_ATTR_MERG, // south stairs up

    [0x3d] = TILE_ATTR_WALK, // in-room stairs that do not change BG
    [0x3e] = TILE_ATTR_NODE | TILE_ATTR_STRS, // in-room stairs ^-shaped
    [0x3f] = TILE_ATTR_NODE | TILE_ATTR_STRS, // in-room stairs?

    [0x40] = TILE_ATTR_WALK,
    [0x48] = TILE_ATTR_WALK,
    [0x4B] = TILE_ATTR_NODE | TILE_ATTR_DOOR,

    [0x50] = TILE_ATTR_LFT0,
    [0x51] = TILE_ATTR_LFT0,
    [0x52] = TILE_ATTR_LFT1,
    [0x53] = TILE_ATTR_LFT2,
    [0x54] = 0, //TILE_ATTR_LFT0,
    [0x55] = TILE_ATTR_LFT1,
    [0x56] = TILE_ATTR_LFT2,
    [0x57] = TILE_ATTR_BONK,

    // Chest, Big Chest, Big Key Block
    [0x58] = TILE_ATTR_CHST,
    [0x59] = TILE_ATTR_CHST,
    [0x5A] = TILE_ATTR_CHST,
    [0x5B] = TILE_ATTR_CHST,
    [0x5C] = TILE_ATTR_CHST,
    [0x5D] = TILE_ATTR_CHST,

    [0x5E] = TILE_ATTR_NODE | TILE_ATTR_DOOR, // stairs up
    [0x5F] = TILE_ATTR_NODE | TILE_ATTR_DOOR, // stairs down

    // Special pots or pushable blocks in a room
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

    [0x80] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR, // open door?
    [0x81] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR,
    [0x82] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR, // lockable door?
    [0x83] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR,
    [0x84] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR,
    [0x85] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR, // locked door?
    [0x86] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR,
    [0x87] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR,
    [0x88] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR,
    [0x89] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR,
    [0x8A] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR,
    [0x8B] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR,
    [0x8C] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR,
    [0x8D] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR,
    [0x8E] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR,
    [0x8F] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR,

    [0x90] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR, // open door?
    [0x91] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR,
    [0x92] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR, // lockable door?
    [0x93] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR,
    [0x94] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR,
    [0x95] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR, // locked door?
    [0x96] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR,
    [0x97] = /* TILE_ATTR_WALK | */ TILE_ATTR_NODE | TILE_ATTR_DOOR,

    [0xA0] = TILE_ATTR_WALK,
    [0xA1] = TILE_ATTR_WALK,
    [0xA2] = TILE_ATTR_WALK,
    [0xA3] = TILE_ATTR_WALK,
    [0xA4] = TILE_ATTR_WALK,
    [0xA5] = TILE_ATTR_WALK,

    [0xF0] = TILE_ATTR_DOOR | TILE_ATTR_NODE, // Locked door, 0x8000?
    [0xF1] = TILE_ATTR_DOOR | TILE_ATTR_NODE, // Locked door, 0x4000?
    [0xF2] = TILE_ATTR_DOOR | TILE_ATTR_NODE, // Locked door
    [0xF3] = TILE_ATTR_DOOR | TILE_ATTR_NODE, // Locked door
    [0xF4] = TILE_ATTR_DOOR | TILE_ATTR_NODE, // Locked door
    [0xF5] = TILE_ATTR_DOOR | TILE_ATTR_NODE, // Locked door
    [0xF6] = TILE_ATTR_DOOR | TILE_ATTR_NODE, // Locked door
    [0xF7] = TILE_ATTR_DOOR | TILE_ATTR_NODE, // Locked door
    [0xF8] = TILE_ATTR_DOOR | TILE_ATTR_NODE, // Locked door, 0x8000?
    [0xF9] = TILE_ATTR_DOOR | TILE_ATTR_NODE, // Locked door, ???
    [0xFA] = TILE_ATTR_DOOR | TILE_ATTR_NODE, // Locked door, ???
    // Fake/unknown
    [0xFF] = TILE_ATTR_WALK,
};

enum ap_room_layout {
    // Cells are laid out as follows:
    // A B
    // C D
    // Undercores separate rooms; ABCD is 1 2x2 room; A_B_C_D is 4 1x1 rooms
    ROOM_LAYOUT_A_B_C_D = 0,
    ROOM_LAYOUT_AC_BD = 1,
    ROOM_LAYOUT_A_BD_C = 2,
    ROOM_LAYOUT_AC_B_D = 3,
    ROOM_LAYOUT_AB_CD = 4,
    ROOM_LAYOUT_A_B_CD = 5,
    ROOM_LAYOUT_AB_C_D = 6,
    ROOM_LAYOUT_ABCD = 7,

    //DUNGEON_LAYOUT_RIGHT = 0x1,
    //DUNGEON_LAYOUT_BOTTOM = 0x2,
};

enum ap_link_state {
    LINK_STATE_GROUND = 0x0,
    LINK_STATE_FALLING_HOLE = 0x1,
    LINK_STATE_SWIMMING = 0x4,
    LINK_STATE_FALLING_LEDGE = 0x11,
    LINK_STATE_RECVING_ITEM = 0x15,
    LINK_STATE_RECVING_ITEM2 = 0x17, // XXX delete this?
    LINK_STATE_HOLDING_BIG_ROCK = 0x18,
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
    0x18 - stuck holding a heavy rock
    0x19 - Receiving Ether Medallion
    0x1A - Receiving Bombos Medallion
    0x1B - Opening Desert Palace
    0x1C - temporary bunny
    0x1D - Rolling back from Gargoyle gate or PullForRupees object
    0x1E - The actual spin attack motion.
*/

#define SPRITE_ATTR_LIST \
    X(ENMY) /* Enemy, can hurt */ \
    X(BLKS) /* Blocked center 2x2 square */ \
    X(BLKF) /* Blocked full sprite size */ \
    X(SWCH) /* Button / state-changer */ \
    X(ITEM) /* Pick-up */ \
    X(TALK) /* Talk for progression item */ \
    X(FLLW) /* Thing that can follow you */ \
    X(NODE) /* Worth making a node for */ \
    X(SUBT) /* Has subtype flags */ \
    X(NVUL) /* Invulnerable, or at least not worth fighting */ \
    X(VBOW) /* Only vunerable to the Bow */ \

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

#define N_SPRITES ((size_t) 16)
extern struct ap_sprite {
    uint8_t type;
    uint16_t subtype;
    uint8_t state;
    uint8_t interaction;
    int8_t hp;
    uint8_t drop;

    bool active;

    struct xy tl;
    struct xy br;
    struct xy hitbox_tl;
    struct xy hitbox_br;
    uint16_t attrs;
} ap_sprites[N_SPRITES];
extern bool ap_sprites_changed;

extern struct ap_pushblock {
    struct xy tl;
} ap_pushblocks[16];

void ap_sprites_update();

extern const char * const ap_sprite_attr_names[];
const char * ap_sprite_attr_name(uint16_t attr);
void ap_sprites_print();
uint16_t ap_sprite_attrs_for_type(uint8_t type, uint16_t subtype, uint16_t dungeon_room);

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
    [0x0D] = SPRITE_ATTR_ENMY | SPRITE_ATTR_NVUL, // Normal Buzzblob / Morphed Buzzblob (tra la la... look for Sahashrala)
    [0x0E] = SPRITE_ATTR_ENMY, // Plants with big mouths
    [0x0F] = SPRITE_ATTR_ENMY, // Octobaloon (Probably the thing that explodes into 10 others)

    [0x10] = SPRITE_ATTR_ENMY, // Small things from the exploder (exploder? wtf is that?)
    [0x11] = SPRITE_ATTR_ENMY, // One Eyed Giants (bomb throwers) aka Hinox
    [0x12] = SPRITE_ATTR_ENMY, // Moblin Spear Throwers
    [0x13] = SPRITE_ATTR_ENMY, // Helmasaur?
    [0x14] = SPRITE_ATTR_BLKF, // Gargoyles Domain Gate
    [0x15] = SPRITE_ATTR_ENMY, // Fire Faery
    [0x16] = SPRITE_ATTR_SUBT, // Sahashrala / Aginah, sage of the desert
    [0x17] = SPRITE_ATTR_ENMY, // Water Bubbles?
    [0x18] = SPRITE_ATTR_ENMY, // Moldorm
    [0x19] = SPRITE_ATTR_ENMY, // Poe
    [0x1A] = 0, // Dwarf, Mallet, and the shrapnel from it hitting
    [0x1B] = 0, // Arrow in wall?
    [0x1C] = SPRITE_ATTR_BLKS, // Moveable Statue
    [0x1D] = SPRITE_ATTR_BLKF, // Weathervane
    [0x1E] = SPRITE_ATTR_BLKF | SPRITE_ATTR_SWCH, // Crystal Switch
    [0x1F] = SPRITE_ATTR_BLKF, // Sick Kid with Bug Catching Net

    [0x20] = SPRITE_ATTR_ENMY, // Bomb Slugs
    [0x21] = SPRITE_ATTR_SWCH, // Push Switch (like in Swamp Palace)
    [0x22] = SPRITE_ATTR_ENMY, // Darkworld Snakebasket
    [0x23] = SPRITE_ATTR_SWCH, // Red Onoff
    [0x24] = SPRITE_ATTR_SWCH, // Blue Onoff
    [0x25] = SPRITE_ATTR_BLKF, // Tree you can talk to?
    [0x26] = SPRITE_ATTR_ENMY, // Charging Octopi?
    [0x27] = SPRITE_ATTR_ENMY, // Dead Rocks? (Gorons bleh)
    [0x28] = SPRITE_ATTR_BLKF, // Shrub Guy who talks about Triforce / Other storytellers
    [0x29] = SPRITE_ATTR_BLKS, // Blind Hideout attendant, Lost woods
    [0x2A] = SPRITE_ATTR_BLKS, // Sweeping Lady
    [0x2B] = SPRITE_ATTR_BLKS, // Bum under the bridge + smoke and other effects like the fire
    [0x2C] = SPRITE_ATTR_BLKF, // Lumberjack Bros. 
    [0x2D] = 0, // Telepathic stones?
    [0x2E] = 0, // Flute Boys Notes
    [0x2F] = SPRITE_ATTR_BLKS, // Heart Piece Race guy and girl
    [0x30] = SPRITE_ATTR_BLKF, // Person? (HM name)
    [0x31] = SPRITE_ATTR_BLKS, // Fortune Teller / Dwarf swordsmith
    [0x32] = SPRITE_ATTR_BLKF, // ??? (something with a turning head) 
    [0x33] = 0, // Pull the wall for rupees
    [0x34] = SPRITE_ATTR_BLKF, // ScaredGirl2 (HM name)
    [0x35] = SPRITE_ATTR_BLKS, // Innkeeper
    [0x36] = SPRITE_ATTR_BLKF, // Witch / Cane of Byrna?
    [0x37] = 0, // Waterfall
    [0x38] = SPRITE_ATTR_BLKF, // Arrow Target (e.g. Big Eye in Dark Palace)
    [0x39] = SPRITE_ATTR_BLKF, // Middle Aged Guy in the desert
    [0x3A] = 0, // Magic Powder Bat /The Lightning Bolt the bat hurls at you.
    [0x3B] = 0, // Dash Item / such as Book of Mudora, keys
    [0x3C] = SPRITE_ATTR_BLKS, // Kid in village near the trough
    [0x3D] = SPRITE_ATTR_BLKS, // Signs? Chicken lady also showed up / Scared ladies outside houses.
    [0x3E] = SPRITE_ATTR_ENMY, // Rock Rupee Crabs
    [0x3F] = SPRITE_ATTR_ENMY, // Tutorial Soldiers from beginning of game
    
    [0x40] = SPRITE_ATTR_ENMY /*| SPRITE_ATTR_BLKF */, // Hyrule Castle Barrier to Agahnims Tower
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
    
    [0x50] = SPRITE_ATTR_ENMY | SPRITE_ATTR_NVUL, // Metal Balls (in Eastern Palace)
    [0x51] = SPRITE_ATTR_ENMY, // Armos
    [0x52] = 0, // Giant Zora
    [0x53] = SPRITE_ATTR_ENMY | SPRITE_ATTR_VBOW, // Armos Knights Boss
    [0x54] = SPRITE_ATTR_ENMY, // Lanmolas boss
    [0x55] = SPRITE_ATTR_ENMY, // Zora / Fireballs (including the blue Agahnim fireballs)
    [0x56] = SPRITE_ATTR_ENMY, // Walking Zora
    [0x57] = SPRITE_ATTR_BLKF, // Desert Palace Barriers
    [0x58] = SPRITE_ATTR_ENMY, // Sandcrab
    [0x59] = SPRITE_ATTR_ENMY, // Birds (boids)
    [0x5A] = SPRITE_ATTR_ENMY, // Squirrels
    [0x5B] = SPRITE_ATTR_ENMY, // Energy Balls (that crawl along the wall)
    [0x5C] = SPRITE_ATTR_ENMY, // Wall crawling fireballs
    [0x5D] = SPRITE_ATTR_ENMY, // Roller (vertical moving)
    [0x5E] = SPRITE_ATTR_ENMY, // Roller (vertical moving)
    [0x5F] = SPRITE_ATTR_ENMY, // Roller
    
    [0x60] = SPRITE_ATTR_ENMY, // Roller (horizontal moving)
    [0x61] = SPRITE_ATTR_BLKF, // Statue Sentry
    [0x62] = SPRITE_ATTR_BLKF, // Master Sword plus pendants and beams of light
    [0x63] = 0, // Sand Lion Pit
    [0x64] = SPRITE_ATTR_ENMY, // Sand Lion
    [0x65] = 0, // Shooting Gallery guy
    [0x66] = SPRITE_ATTR_BLKF, // Moving cannon ball shooters
    [0x67] = SPRITE_ATTR_BLKF, // Moving cannon ball shooters
    [0x68] = SPRITE_ATTR_BLKF, // Moving cannon ball shooters
    [0x69] = SPRITE_ATTR_BLKF, // Moving cannon ball shooters 
    [0x6A] = SPRITE_ATTR_ENMY, // Ball N' Chain Trooper
    [0x6B] = SPRITE_ATTR_ENMY, // Cannon Ball Shooting Soldier (unused in original = WTF?)
    [0x6C] = 0, // Warp Vortex created by Magic Mirror
    [0x6D] = SPRITE_ATTR_ENMY, // Mouse
    [0x6E] = SPRITE_ATTR_ENMY, // Snakes (forgot the Zelda 1 name)
    [0x6F] = SPRITE_ATTR_ENMY, // Bats / Also one eyed bats
    
    [0x70] = SPRITE_ATTR_ENMY, // Splitting Fireballs from Helmasaur King
    [0x71] = SPRITE_ATTR_SWCH, // Leever
    [0x72] = 0, // Activator for the ponds (where you throw in items)
    [0x73] = SPRITE_ATTR_SUBT, // Links Uncle / Sage / Barrier that opens in the sanctuary
    [0x74] = 0, // Red Hat Boy who runs from you
    [0x75] = SPRITE_ATTR_TALK | SPRITE_ATTR_BLKS, // Bottle Vendor
    [0x76] = SPRITE_ATTR_BLKF | SPRITE_ATTR_FLLW, // Princess Zelda
    [0x77] = SPRITE_ATTR_ENMY, // Also Fire Faeries (seems like a different variety)
    [0x78] = SPRITE_ATTR_BLKS, // Village Elder
    [0x79] = 0, // Good bee / normal bee
    [0x7A] = SPRITE_ATTR_ENMY /*| SPRITE_ATTR_NVUL*/, // Agahnim
    [0x7B] = SPRITE_ATTR_ENMY, // Agahnim energy blasts (not the duds)
    [0x7C] = 0, // Boos? As in the Boos from SM3/SMW? Thats crazy talk!
    [0x7D] = 0, // 32*32 Pixel Yellow Spike Traps
    [0x7E] = SPRITE_ATTR_ENMY, // Swinging Fireball Chains
    [0x7F] = SPRITE_ATTR_ENMY, // Swinging Fireball Chains
    
    [0x80] = SPRITE_ATTR_ENMY, // Wandering Fireball Chains
    [0x81] = SPRITE_ATTR_ENMY, // Waterhoppers
    [0x82] = SPRITE_ATTR_ENMY, // Swirling Fire Faeries (Eastern Palace)
    [0x83] = SPRITE_ATTR_ENMY, // Green Rocklops (igor, eyegor)
    [0x84] = SPRITE_ATTR_ENMY | SPRITE_ATTR_VBOW, // Red Rocklops
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
    [0x9F] = 0, //SPRITE_ATTR_ITEM | SPRITE_ATTR_NODE, // Flute
    
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
    [0xB5] = SPRITE_ATTR_BLKF | SPRITE_ATTR_TALK, // Bomb Salesman (elephant looking guy)
    [0xB6] = SPRITE_ATTR_FLLW, // Kiki the monkey?
    [0xB7] = SPRITE_ATTR_FLLW, // Maiden following you in Blind Dungeon
    [0xB8] = 0, // Monologue Testing Sprite (Debug Artifact)
    [0xB9] = 0, // Feuding Friends on Death Mountain
    [0xBA] = 0, // Whirlpool
    // subtype2: 0x00 salesman; 0x0c bombs; 0x0a one heart; 0x07 red potion
    [0xBB] = SPRITE_ATTR_SUBT | SPRITE_ATTR_BLKF | SPRITE_ATTR_TALK, // Salesman / chestgame guy / 300 rupee giver guy / Chest game thief / Item for sale
    [0xBC] = SPRITE_ATTR_BLKF, // Drunk in the inn
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
    [0xD5] = SPRITE_ATTR_BLKF | SPRITE_ATTR_TALK, // Digging Game Proprietor
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
    [0xE4] = SPRITE_ATTR_ITEM | SPRITE_ATTR_NODE, // Key
    [0xE5] = SPRITE_ATTR_ITEM | SPRITE_ATTR_NODE, // Big Key
    [0xE6] = SPRITE_ATTR_ITEM | SPRITE_ATTR_NODE, // Red Shield (after dropped by pickit)
    [0xE7] = SPRITE_ATTR_ITEM | SPRITE_ATTR_NODE, // Mushroom
    [0xE8] = 0, // Fake Master Sword
    [0xE9] = 0, // Magic Shop dude / His items, including the magic powder
    [0xEA] = SPRITE_ATTR_ITEM | SPRITE_ATTR_NODE, // Full Heart Container
    [0xEB] = SPRITE_ATTR_ITEM | SPRITE_ATTR_NODE, // Quarter Heart Container
    [0xEC] = 0, // Bushes or pot, picked up/thrown
    [0xED] = 0, // Cane of Somaria Platform
    [0xEE] = SPRITE_ATTR_BLKF, // Movable Mantle (in Hyrule Castle)
    [0xEF] = 0, // Cane of Somaria Platform (same as 0xED but this index is not used)
    
    [0xF0] = 0, // Cane of Somaria Platform (same as 0xED but this index is not used)
    [0xF1] = 0, // Cane of Somaria Platform (same as 0xED but this index is not used)
    [0xF2] = SPRITE_ATTR_BLKF, // Medallion Tablet
};

struct ap_sprite_subtype {
    uint8_t type;
    uint16_t subtype;
    uint16_t attrs;
    uint16_t only_dungeon_room;
};

static const struct ap_sprite_subtype ap_sprite_subtypes[] = {
    // Sahashrala
    { .type = 0x16, .subtype = 0x0000, .attrs = SPRITE_ATTR_BLKF | SPRITE_ATTR_TALK | SPRITE_ATTR_NODE },
    // Desert Sage
    { .type = 0x16, .subtype = 0x0100, .attrs = SPRITE_ATTR_BLKF },

    // Barrier in Sanctuary
    { .type = 0x73, .subtype = 0x0000, .only_dungeon_room = 0x12, .attrs = SPRITE_ATTR_BLKF },
    { .type = 0x73, .subtype = 0x0100, .only_dungeon_room = 0x12, .attrs = SPRITE_ATTR_BLKF },
    // Link's Uncle
    { .type = 0x73, .subtype = 0x0100, .only_dungeon_room = 0x55, .attrs = SPRITE_ATTR_TALK | SPRITE_ATTR_NODE},
    // Guy next to Zelda
    { .type = 0x73, .subtype = 0x0200, .attrs = SPRITE_ATTR_BLKS },

    // Mini Moldorm Cave Guy
    { .type = 0xBB, .subtype = 0x0200, .only_dungeon_room = 0x123, .attrs = SPRITE_ATTR_NODE | SPRITE_ATTR_TALK },
};

#define DOOR_ATTR_LIST \
    X(NORM) /* Normal; just to differentiate unknown vs. unremarkable */ \
    X(BOMB) /* Bombable */ \
    X(SKEY) /* Openable with small key */ \
    X(BKEY) /* Openable with big key */ \
    X(SLSH) /* Openable with sword */ \
    X(TRAP) /* Becomes a trap door */ \
    X(BLOK) /* Not actually a door */ \

enum {
#define X(d) CONCAT(_DOOR_ATTR_INDEX_, d),
DOOR_ATTR_LIST
#undef X
};
enum {
#define X(d) CONCAT(DOOR_ATTR_, d) = 1ul << CONCAT(_DOOR_ATTR_INDEX_, d),
DOOR_ATTR_LIST
#undef X
};

const char * ap_door_attr_name(uint16_t idx);

static const uint16_t ap_door_attrs[256] = {
    [0x00] = DOOR_ATTR_NORM,
    [0x01] = DOOR_ATTR_NORM,
    [0x02] = DOOR_ATTR_NORM,
    [0x04] = DOOR_ATTR_NORM,
    [0x05] = DOOR_ATTR_NORM,
    [0x06] = DOOR_ATTR_NORM,
    [0x07] = DOOR_ATTR_NORM,
    [0x08] = DOOR_ATTR_NORM,
    [0x09] = DOOR_ATTR_NORM,
    [0x0A] = DOOR_ATTR_NORM,
    [0x0B] = DOOR_ATTR_NORM,
    [0x0C] = DOOR_ATTR_TRAP,
    [0x0D] = DOOR_ATTR_BLOK, // ? Only in Turtle rock, "invisible"
    [0x0E] = DOOR_ATTR_SKEY,
    [0x0F] = DOOR_ATTR_BKEY,
    [0x10] = DOOR_ATTR_SKEY,
    [0x11] = DOOR_ATTR_SKEY,
    [0x13] = DOOR_ATTR_SKEY,
    [0x14] = DOOR_ATTR_BOMB, // Confirmed by ASM
    [0x15] = DOOR_ATTR_BOMB, // Confirmed by ASM
    [0x17] = DOOR_ATTR_BOMB, // Confirmed by ASM
    [0x18] = DOOR_ATTR_NORM,
    [0x19] = DOOR_ATTR_SLSH,
    [0x1B] = DOOR_ATTR_TRAP, // ? 'right side only'?
    [0x1C] = DOOR_ATTR_TRAP, // ? 'left side only'?
    [0x20] = DOOR_ATTR_NORM,
    [0x22] = DOOR_ATTR_TRAP, // ? 'left side only'?
    [0x23] = DOOR_ATTR_NORM,
    [0x24] = DOOR_ATTR_TRAP, // ? one side trap?
    [0x25] = DOOR_ATTR_TRAP, // ? one side trap?
    [0x80] = DOOR_ATTR_NORM, // Fake; stairs 0x0E
    [0x81] = DOOR_ATTR_NORM, // Fake; stairs 0x0F
};

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

// current_item (guessing from inventory layout, * are confirmed)
// 0x01 Bow
// 0x02 Boomerang
// 0x03 Hookshot
// 0x04 Bombs *
// 0x05 Powder *
// 0x06 Fire rod
// 0x07 Ice rod
// 0x08 Bombos
// 0x09 Ether
// 0x0a Quake
// 0x0b Lamp
// 0x0c Hammer *
// 0x0d Flute *
// 0x0e Net
// 0x0f Book of Mudora *
// 0x10 Bottles?
// 0x11 Red cane
// 0x12 Blue cane
// 0x13 Magic cape
// 0x14 Mirror *

#define INVENTORY_LIST \
    X(0x01, BOW) \
    X(0x02, BOOMERANG) \
    X(0x03, HOOKSHOT) \
    X(0x04, BOMBS) \
    X(0x05, POWDER) \
    X(0x06, FIRE_ROD) \
    X(0x07, ICE_ROD) \
    X(0x08, BOMBOS) \
    X(0x09, ETHER) \
    X(0x0a, QUAKE) \
    X(0x0b, LAMP) \
    X(0x0c, HAMMER) \
    X(0x0d, FLUTE) \
    X(0x0e, NET) \
    X(0x0f, BOOK) \
    /*X(0x10, BOTTLES) -- this is just boolean 'has bottles' */ \
    X(0x11, RED_CANE) \
    X(0x12, BLUE_CANE) \
    X(0x13, MAGIC_CAPE) \
    X(0x14, MIRROR) \
    X(0x15, GLOVES) \
    X(0x16, BOOTS) \
    X(0x17, FLIPPERS) \
    X(0x18, MOON_PEARL) \
    X(0x1A, SWORD) \
    X(0x1B, SHIELD) \
    X(0x1C, ARMOR) \
    X(0x1D, BOTTLE) /* 4 bottles 0x1D, 0x1E, 0x1F, 0x20 */\

enum ap_inventory {
    _INVENTORY_MIN = 1,
#define X(i, n) CONCAT(INVENTORY_, n) = i,
INVENTORY_LIST
#undef X
    _INVENTORY_MAX
};

extern const char * const ap_inventory_names[];

static const uint16_t ap_ancillia_attrs[256] = {
    // Crystal / Pendant
    [0x29] = SPRITE_ATTR_NODE | SPRITE_ATTR_ITEM,
};

#define N_ANCILLIA ((size_t) 10)
extern struct ap_ancillia {
    uint8_t type;
    uint8_t bf0; // 0xBF0
    struct xy tl;
    struct xy br;
    struct xy subpixel;
    struct xy velocity;
    uint16_t attrs;
} ap_ancillia[N_ANCILLIA];

void ap_ancillia_update();
void ap_ancillia_print();

enum ap_quadrant {
    QUAD_ALL = 0xF,
    QUAD_A = 0x1,
    QUAD_B = 0x2,
    QUAD_C = 0x4,
    QUAD_D = 0x8,
};

const char * ap_quadrant_print(uint8_t quadmask);

#define ROOM_ACTION_LIST \
    X(NONE) \
    X(CLEAR_LEVEL) \
    X(CLEAR_QUADRANT) \
    X(CLEAR_ROOM) \
    X(KILL_ENEMY) \
    X(LIGHT_TORCHES) \
    X(MOVE_BLOCK) \
    X(OPEN_CHEST) \
    X(PULL_LEVER) \
    X(SWITCH_HOLD) \
    X(SWITCH_TOGGLE) \
    X(TURN_OFF_WATER) \
    X(TURN_ON_WATER) \

#define ROOM_RESULT_LIST \
    X(NONE) \
    X(CHEST) \
    X(CLEAR_LEVEL) \
    X(CRASH) \
    X(HOLES) \
    X(MOVE_BLOCK) \
    X(OPEN_DOORS) \
    X(OPEN_WALL) \
    X(WATERGATE) \

enum ap_room_action {
#define X(n) CONCAT(ROOM_ACTION_, n),
ROOM_ACTION_LIST
#undef X
};

enum ap_room_result {
#define X(n) CONCAT(ROOM_RESULT_, n),
ROOM_RESULT_LIST
#undef X
};

static const struct ap_room_tag {
    uint8_t quadmask;
    enum ap_room_action action;
    enum ap_room_result result;
    bool unsure;
} ap_room_tags[0x40] = {
    [0x00] = { .quadmask = 0,               .action = ROOM_ACTION_NONE,             .result = ROOM_RESULT_NONE },

    [0x01] = { .quadmask = QUAD_A,          .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_OPEN_DOORS },
    [0x02] = { .quadmask = QUAD_B,          .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_OPEN_DOORS },
    [0x03] = { .quadmask = QUAD_C,          .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_OPEN_DOORS },
    [0x04] = { .quadmask = QUAD_D,          .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_OPEN_DOORS },
    [0x05] = { .quadmask = QUAD_A | QUAD_C, .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_OPEN_DOORS },
    [0x06] = { .quadmask = QUAD_B | QUAD_D, .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_OPEN_DOORS },
    [0x07] = { .quadmask = QUAD_A | QUAD_B, .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_OPEN_DOORS },
    [0x08] = { .quadmask = QUAD_C | QUAD_D, .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_OPEN_DOORS },
    [0x09] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_CLEAR_QUADRANT,   .result = ROOM_RESULT_OPEN_DOORS, .unsure = true },
    [0x0A] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_CLEAR_ROOM,       .result = ROOM_RESULT_OPEN_DOORS, .unsure = true },

    [0x0B] = { .quadmask = QUAD_A,          .action = ROOM_ACTION_MOVE_BLOCK,       .result = ROOM_RESULT_OPEN_DOORS },
    [0x0C] = { .quadmask = QUAD_B,          .action = ROOM_ACTION_MOVE_BLOCK,       .result = ROOM_RESULT_OPEN_DOORS },
    [0x0D] = { .quadmask = QUAD_C,          .action = ROOM_ACTION_MOVE_BLOCK,       .result = ROOM_RESULT_OPEN_DOORS },
    [0x0E] = { .quadmask = QUAD_D,          .action = ROOM_ACTION_MOVE_BLOCK,       .result = ROOM_RESULT_OPEN_DOORS },
    [0x0F] = { .quadmask = QUAD_A | QUAD_C, .action = ROOM_ACTION_MOVE_BLOCK,       .result = ROOM_RESULT_OPEN_DOORS },
    [0x10] = { .quadmask = QUAD_B | QUAD_D, .action = ROOM_ACTION_MOVE_BLOCK,       .result = ROOM_RESULT_OPEN_DOORS },
    [0x11] = { .quadmask = QUAD_A | QUAD_B, .action = ROOM_ACTION_MOVE_BLOCK,       .result = ROOM_RESULT_OPEN_DOORS },
    [0x12] = { .quadmask = QUAD_C | QUAD_D, .action = ROOM_ACTION_MOVE_BLOCK,       .result = ROOM_RESULT_OPEN_DOORS },

    [0x13] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_MOVE_BLOCK,       .result = ROOM_RESULT_OPEN_DOORS, .unsure = true },
    [0x14] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_PULL_LEVER,       .result = ROOM_RESULT_OPEN_DOORS, .unsure = true },
    [0x15] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_CLEAR_LEVEL,      .result = ROOM_RESULT_OPEN_DOORS, }, // Boss room

    [0x16] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_SWITCH_HOLD,      .result = ROOM_RESULT_OPEN_DOORS },
    [0x17] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_SWITCH_TOGGLE,    .result = ROOM_RESULT_OPEN_DOORS },

    [0x18] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_TURN_OFF_WATER,   .result = ROOM_RESULT_NONE, .unsure = true },
    [0x19] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_TURN_ON_WATER,    .result = ROOM_RESULT_NONE, .unsure = true },
    [0x1A] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_NONE,             .result = ROOM_RESULT_WATERGATE }, // Overworld Dam
    [0x1B] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_NONE,             .result = ROOM_RESULT_WATERGATE, .unsure = true },

    [0x1C] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_NONE,             .result = ROOM_RESULT_OPEN_WALL, .unsure = true },
    [0x1D] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_NONE,             .result = ROOM_RESULT_OPEN_WALL, .unsure = true },
    [0x1E] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_NONE,             .result = ROOM_RESULT_CRASH, .unsure = true },
    [0x1F] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_NONE,             .result = ROOM_RESULT_CRASH, .unsure = true },

    [0x20] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_SWITCH_TOGGLE,    .result = ROOM_RESULT_OPEN_WALL, .unsure = true },
    [0x21] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_NONE,             .result = ROOM_RESULT_HOLES, .unsure = true },
    [0x22] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_OPEN_CHEST,       .result = ROOM_RESULT_HOLES, .unsure = true },
    [0x23] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_NONE,             .result = ROOM_RESULT_HOLES, .unsure = true },
    [0x24] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_NONE,             .result = ROOM_RESULT_HOLES, .unsure = true },

    [0x25] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_CLEAR_LEVEL, }, // Boss room
    [0x26] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_MOVE_BLOCK, .unsure = true },

    [0x27] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_SWITCH_TOGGLE,    .result = ROOM_RESULT_CHEST },
    [0x28] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_PULL_LEVER,       .result = ROOM_RESULT_OPEN_WALL, .unsure = true },

    [0x29] = { .quadmask = QUAD_A,          .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_CHEST },
    [0x2A] = { .quadmask = QUAD_B,          .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_CHEST },
    [0x2B] = { .quadmask = QUAD_C,          .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_CHEST },
    [0x2C] = { .quadmask = QUAD_D,          .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_CHEST },
    [0x2D] = { .quadmask = QUAD_A | QUAD_C, .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_CHEST },
    [0x2E] = { .quadmask = QUAD_B | QUAD_D, .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_CHEST },
    [0x2F] = { .quadmask = QUAD_A | QUAD_B, .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_CHEST },
    [0x30] = { .quadmask = QUAD_C | QUAD_D, .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_CHEST },
    [0x31] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_CLEAR_QUADRANT,   .result = ROOM_RESULT_CHEST },
    [0x32] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_CLEAR_ROOM,       .result = ROOM_RESULT_CHEST },

    [0x33] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_LIGHT_TORCHES,    .result = ROOM_RESULT_OPEN_DOORS, },

    [0x34] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_NONE,             .result = ROOM_RESULT_HOLES, .unsure = true },
    [0x35] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_NONE,             .result = ROOM_RESULT_HOLES, .unsure = true },
    [0x36] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_NONE,             .result = ROOM_RESULT_HOLES, .unsure = true },
    [0x37] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_NONE,             .result = ROOM_RESULT_HOLES, .unsure = true },
    [0x38] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_NONE,             .result = ROOM_RESULT_NONE, },
    [0x39] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_NONE,             .result = ROOM_RESULT_HOLES, .unsure = true },
    [0x3A] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_NONE,             .result = ROOM_RESULT_HOLES, .unsure = true },
    [0x3B] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_OPEN_CHEST,       .result = ROOM_RESULT_HOLES, .unsure = true },

    [0x3C] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_MOVE_BLOCK,       .result = ROOM_RESULT_CHEST, },
    [0x3D] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_OPEN_DOORS, },
    [0x3E] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_LIGHT_TORCHES,    .result = ROOM_RESULT_CHEST, },
    [0x3F] = { .quadmask = QUAD_ALL,        .action = ROOM_ACTION_KILL_ENEMY,       .result = ROOM_RESULT_NONE, .unsure = true},
};

const char * ap_room_tag_print(const struct ap_room_tag * tag);

#define DUNGEON_LIST \
    X(SEWERS, "Sr") \
    X(HYRULE_CASTLE, "HC") \
    X(EASTERN_PALACE, "EP") \
    X(DESERT_PALACE, "DP") \
    X(AGAHANIMS_TOWER, "CT") /* AKA "hyrule castle 2" & "Castle Tower" */ \
    X(SWAMP_PALACE, "SP") \
    X(PALACE_OF_DARKNESS, "PoD") \
    X(MISERY_MIRE, "MM") \
    X(SKULL_WOODS, "SW") \
    X(ICE_PALACE, "IP") \
    X(TOWER_OF_HERA, "ToH") \
    X(THEIVES_TOWN, "TT") /* AKA "gargoyle's domain" */ \
    X(TURTLE_ROCK, "TR") \
    X(GANONS_TOWER, "GT") \

enum ap_dungeon {
#define X(name, abbr) CONCAT(DUNGEON_, name),
DUNGEON_LIST
#undef X
    _DUNGEON_MAX
};

extern const char * const ap_dungeon_names[];
extern const char * const ap_dungeon_abbrs[];
