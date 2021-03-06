#pragma once
#include "ap_macro.h"
#include "ap_math.h"
#include "ap_snes.h"

//                                   Matches ledge direction
//                                   0   U   D   L   R  LU  LD  RU  RD  0
#define DIR_LIST   X(U)   X(D)   X(L)   X(R)   X(LU)   X(LD)   X(RU)   X(RD)
enum { DIR_NONE, DIR_U, DIR_D, DIR_L, DIR_R, DIR_LU, DIR_LD, DIR_RU, DIR_RD };
static const int8_t dir_dx[10]    = {0,  0,  0, -1, +1, -1, -1, +1, +1, 0};
static const int8_t dir_dy[10]    = {0, -1, +1,  0,  0, -1, +1, -1, +1, 0};
static const uint8_t dir_cost[10] = {0, 32, 32, 32, 32, 48, 48, 48, 48, 0}; // cost to move 4x4 blocks

static const uint16_t dir_joypad[10] = {
    [DIR_U] = SNES_UP_MASK, [DIR_D] = SNES_DOWN_MASK,
    [DIR_L] = SNES_LEFT_MASK, [DIR_R] = SNES_RIGHT_MASK,
    [DIR_LU] = SNES_UP_MASK | SNES_LEFT_MASK, [DIR_LD] = SNES_DOWN_MASK | SNES_LEFT_MASK,
    [DIR_RU] = SNES_UP_MASK | SNES_RIGHT_MASK, [DIR_RD] = SNES_DOWN_MASK | SNES_RIGHT_MASK,
};

static const struct xy dir_dxy[10] = {
    [DIR_U] = XY(0, -1), [DIR_D] = XY(0, +1),
    [DIR_L] = XY(-1, 0), [DIR_R] = XY(+1, 0),
    [DIR_LU] = XY(-1, -1), [DIR_LD] = XY(-1, +1),
    [DIR_RU] = XY(+1, -1), [DIR_RD] = XY(+1, +1),
};
static const uint8_t dir_opp[10] = {
    [DIR_U] = DIR_D, [DIR_D] = DIR_U,
    [DIR_L] = DIR_R, [DIR_R] = DIR_L,
    [DIR_LU] = DIR_RD, [DIR_RD] = DIR_LU,
    [DIR_RU] = DIR_LD, [DIR_LD] = DIR_RU,
};
static const uint8_t dir_cw[10] = {
    [DIR_U] = DIR_R, [DIR_D] = DIR_L,
    [DIR_L] = DIR_U, [DIR_R] = DIR_D,
};
static const uint8_t dir_ccw[10] = {
    [DIR_U] = DIR_L, [DIR_D] = DIR_R,
    [DIR_L] = DIR_D, [DIR_R] = DIR_D,
};

static const char * const dir_names[10] = {
    [0] = "0",
#define X(d) [CONCAT(DIR_, d)] = STRINGIFY(d),
DIR_LIST
#undef X
};

enum {
#define X(d) CONCAT(DIRBIT_, d) = (1ul << (CONCAT(DIR_, d) - 1)),
DIR_LIST
#undef X
};

#define NODE_TYPE_LIST  \
    X(NONE) \
    X(ITEM) \
    X(CHEST) \
    X(TRANSITION) \
    X(SWITCH) \
    X(SPRITE) \
    X(SCRIPT) \
    X(OVERLAY) \
    X(KEYBLOCK) \

struct ap_screen;
struct ap_node {
    struct ap_screen * screen;
    struct ap_node * next;
    struct ap_node * prev;

    struct xy tl;
    struct xy br;

    uint8_t adjacent_direction;
    //struct ap_screen ** adjacent_screen;
    struct ap_node * adjacent_node;
    struct xy locked_xy; // door is locked if this is 0xFx not 0x82
    struct ap_node * lock_node;
    const struct ap_script * script;
    struct ap_goal * goal;
    struct ap_item_loc * item_loc;
    uint8_t overlay_index;

    enum ap_node_type {
#define X(type) CONCAT(NODE_, type),
NODE_TYPE_LIST
#undef X
    } type;
    uint8_t tile_attr;
    uint8_t sprite_type;
    uint16_t sprite_subtype;
    union {
        uint8_t door_type;
        uint8_t chest_type;
    };
    bool _reachable;
    bool _debug_blocked;
    char name[32];

    // Global Search State
    struct ap_node_pgsearch {
        uint64_t iter;
        //struct xy xy;
        struct ap_node * from;
        uint64_t distance;
    } pgsearch;
};

#define PRINODE "%s%s%s"
#define PRINODEF(n) ((n) ? (n)->name : "(null)"), ((n) && (n)->screen ? " screen=" : ""), ((n) && (n)->screen ? (n)->screen->name : "")

struct ap_screen {
    struct xy tl;
    struct xy br;
    uint8_t quadmask;
    uint16_t id;
    uint8_t dungeon_id;
    uint16_t dungeon_room;
    uint16_t dungeon_tags;
    const struct ap_room_tag * room_tags[2];
    struct ap_node node_list[1];
    const struct ap_screen_info * info;
    char name[64];
    uint8_t attr_cache[0x80][0x80];
    //struct ap_graph graph;

    struct ap_node_distance {
        struct ap_node *src;
        struct ap_node *dst;
        uint64_t distance;
    } * distances;
    size_t distances_length;
    size_t distances_capacity;
};

struct ap_script {
    struct xy start_tl;
    int start_item;
    const char * sequence;
    enum script_type {
        SCRIPT_SEQUENCE,
        SCRIPT_KILLALL,
        SCRIPT_KILLDROPS,
    } type;
    char name[32];
};

struct xy
ap_link_xy();

struct xy
ap_sprite_xy(uint8_t i);

uint16_t
ap_map_attr(struct xy xy);

void
ap_map_bounds(struct xy * topleft, struct xy * bottomright);

struct xy
ap_map16_to_xy(struct xy tl, uint16_t map16);

void
ap_print_map_screen(struct ap_screen * screen);

void
ap_print_map_screen_pair(void);

void
ap_print_map_graph();

void
ap_print_map_full(void);

void
ap_print_state(void);

uint32_t
ap_path_heuristic(struct xy src, struct xy dst_tl, struct xy dst_br);

int
ap_follow_targets(uint16_t * joypad, enum ap_inventory * equip_out);

void
ap_joypad_setdir(uint16_t * joypad, uint8_t dir);

struct ap_screen *
ap_update_map_screen(bool force);

int
ap_pathfind_node(struct ap_node * node, bool commit, int max_distance);

int
ap_pathfind_sprite(size_t sprite_idx);

int
ap_set_script(const struct ap_script * script);

int
ap_map_record_transition_from(struct ap_node * src_node);

bool
ap_node_islocked(struct ap_node * node, bool *unlockable_out, const struct ap_room_tag **  unlock_tag_out);

void
ap_node_islocked_print(struct ap_node * node);

void
ap_map_import(const char * filename);

void
ap_map_export(const char * filename);

void
ap_map_tick(void);
