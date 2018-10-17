#pragma once
#include "ap_macro.h"
#include "ap_math.h"

//                                   Matches ledge direction
//                                   0   U   D   L   R  LU  LD  RU  RD  0
#define DIR_LIST   X(U)   X(D)   X(L)   X(R)   X(LU)   X(LD)   X(RU)   X(RD)
enum { DIR_NONE, DIR_U, DIR_D, DIR_L, DIR_R, DIR_LU, DIR_LD, DIR_RU, DIR_RD };
static const int8_t dir_dx[10]    = {0,  0,  0, -1, +1, -1, -1, +1, +1, 0};
static const int8_t dir_dy[10]    = {0, -1, +1,  0,  0, -1, +1, -1, +1, 0};
static const uint8_t dir_cost[10] = {0, 32, 32, 32, 32, 48, 48, 48, 48, 0}; // cost to move 4x4 blocks

static const struct xy dir_dxy[10] = {
    [DIR_U] = XY(0, -1), [DIR_D] = XY(0, +1),
    [DIR_L] = XY(-1, 0), [DIR_R] = XY(+1, 0),
    [DIR_LU] = XY(-1, -1), [DIR_LD] = XY(-1, +1),
    [DIR_RU] = XY(+1, -1), [DIR_RD] = XY(+1, +1),
};


struct ap_node {
    struct ap_node * node_parent;
    struct ap_node * next;
    struct ap_node * prev;
    struct ap_node * node_adjacent;
    uint8_t adjacent_direction;
    struct xy tl;
    struct xy br;
    uint8_t tile_attr;
    char name[16];
    bool _reachable;
    struct ap_node * _plan_from;
};

struct xy
ap_link_xy();

void
ap_map_bounds(struct xy * topleft, struct xy * bottomright);

void
ap_print_map();

uint32_t
ap_path_heuristic(struct xy src, struct xy dst_tl, struct xy dst_br);

int
ap_follow_targets(uint16_t * joypad);

void
ap_joypad_setdir(uint16_t * joypad, uint8_t dir);

struct ap_node *
ap_update_map_node();

int
ap_pathfind_node(struct ap_node * node);
