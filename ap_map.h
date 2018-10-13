#pragma once
#include "ap_macro.h"
#include "ap_math.h"

//                                   0   L   U   R   D  LU  RU  LD  RD  0
static const int8_t dir_dx[10]    = {0, -1,  0,  1,  0, -1,  1, -1,  1, 0};
static const int8_t dir_dy[10]    = {0,  0, -1,  0,  1, -1, -1,  1,  1, 0};
static const uint8_t dir_cost[10] = {0, 32, 32, 32, 32, 48, 48, 48, 48, 0}; // cost to move 4x4 blocks

struct ap_node {
    struct ap_node * node_parent;
    struct ap_node * node_next;
    struct ap_node * node_prev;
    struct ap_node * node_adjacent;
    uint8_t adjacent_direction;
    struct xy tl;
    struct xy br;
    char name[16];
};

struct xy
ap_link_xy();

void
ap_map_bounds(struct xy * topleft, struct xy * bottomright);

void
ap_print_map();

int
ap_follow_targets(uint16_t * joypad);

void
ap_update_map_node();
