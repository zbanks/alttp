#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "alttp2_public.h"
#include "../alttp/alttp_constants.h"
#include "pq.h"

#define CONCAT(x, y) CONCAT2(x, y)
#define CONCAT2(x, y) x ## y

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

#define ABSDIFF(x, y) (((x) < (y)) ? ((y) - (x)) : ((x) - (y)))
#define ABS(x)    ((x) < 0 ? -(x) : (x))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define CAST16(x) ((uint16_t) (x))

#define TERM_BOLD(x) "\033[1m" x "\033[0m"

struct xy {
    uint16_t x;
    uint16_t y;
};

//                      0   L   U   R   D  LU  RU  LD  RD  0
int8_t dir_dx[10]    = {0, -1,  0,  1,  0, -1,  1, -1,  1, 0};
int8_t dir_dy[10]    = {0,  0, -1,  0,  1, -1, -1,  1,  1, 0};
uint8_t dir_cost[10] = {0, 32, 32, 32, 32, 48, 48, 48, 48, 0}; // cost to move 4x4 blocks

#define XY(_x, _y) ((struct xy) { .x = CAST16(_x), .y = CAST16(_y) })
#define XYOP1(a, op) XY(((a).x op), ((a).y op))
#define XYOP2(a, op, b) XY(((a).x op (b).x), ((a).y op (b).y))
#define XYL1DIST(a, b) ((int) ABSDIFF((a).x, (b).x) + (int) ABSDIFF((a).y, (b).y))
#define XYMAP8(xy) ((((xy).x & 0x1F8) >> 3) | (((xy).y) & 0x1F8) << 3)
#define XYMAPNODE(xy) ( ((xy).x >> 8) | ((xy).y & 0xFF00) )
#define XYMID(a, b) XYOP2(XYOP1(a, / 2), +, XYOP1(b, / 2))
#define XYIN(xy, tl, br) ((xy).x >= (tl).x && (xy).y >= (tl).y && (xy).x <= (br).x && (xy).y <= (br).y)

//#define PRIXY "(%u %#x, %u %#x)"
//#define PRIXYF(xy) (xy).x, (xy).x, (xy).y, (xy).y
//#define PRIXY "%u,%u"
#define PRIXY "%x,%x"
#define PRIXYF(xy) (xy).x, (xy).y

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

static struct ap_node map_nodes[0x100 * 0x100];
static size_t last_map_node_idx = -1;

struct ap_node *
ap_node_append(struct ap_node * parent) {
    struct ap_node * child = calloc(1, sizeof *child);
    assert(child != NULL);

    child->node_parent = parent;
    child->node_prev = parent->node_prev;
    child->node_prev->node_next = child;
    child->node_next = parent;
    child->node_next->node_prev = child;
    return child;
}

#define DEBUG(fmt, ...) ({ if(ap_debug){printf(fmt "\n", ## __VA_ARGS__);} })
#define INFO(...) snprintf(info_string, sizeof(info_string), __VA_ARGS__)
#define INFO_HEXDUMP(ptr) INFO_HEXDUMP2(((uint8_t *)ptr))
#define INFO_HEXDUMP2(p) INFO("%02X %02X %02X %02X.%02X %02X %02X %02X.%02X %02X %02X %02X", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11]); 

#define ALTTP_RAM_LIST \
    X(frame_counter,   uint8_t,   0x7E001A)   \
    X(in_building,     uint8_t,   0x7E001B)   \
    X(area,            uint8_t,   0x7E008A)   \
    X(link_x,          uint16_t,  0x7E0022)   \
    X(link_y,          uint16_t,  0x7E0020)   \
    X(link_hold,       uint16_t,  0x7E0308)   \
    X(link_direction,  uint8_t,   0x7E002F)   \
    X(dungeon_room,    uint16_t,  0x7E00A0)   \
    X(menu_part,       uint8_t,   0x7E00C8)   \
    X(sprite_y_lo,     uint8_t,   0x7E0D00)   \
    X(sprite_x_lo,     uint8_t,   0x7E0D10)   \
    X(sprite_y_hi,     uint8_t,   0x7E0D20)   \
    X(sprite_x_hi,     uint8_t,   0x7E0D30)   \
    X(sprite_type,     uint8_t,   0x7E0E20)   \
    X(map_area,        uint16_t,  0x7E040A)   \
    X(map_y_offset,    uint16_t,  0x7E0708)   \
    X(map_y_mask,      uint16_t,  0x7E070A)   \
    X(map_x_offset,    uint16_t,  0x7E070C)   \
    X(map_x_mask,      uint16_t,  0x7E070E)   \
    X(map_area_size,   uint16_t,  0x7E0712)   \
    X(over_map16,      uint16_t,  0x7E2000)   \
    X(dngn_bg1_map8,   uint16_t,  0x7E2000)   \
    X(dngn_bg2_map8,   uint16_t,  0x7E4000)   \
    X(map16_to_map8,   uint64_t,  0x078000)   \
    X(chr_to_tattr,    uint64_t,  0x078000)   \
    X(dngn_bg2_tattr,  uint8_t,   0x7F2000)   \
    X(dngn_bg1_tattr,  uint8_t,   0x7F3000)   \
    X(over_tattr,      uint8_t,   0x1BF110)   \
    X(over_tattr2,     uint8_t,   0x0FFD94)   \
    X(transition_dir,  uint16_t,  0x7E0410)   \
    X(layers_enabled,  uint8_t,   0x7E001C)   \
    X(vram_transfer,   uint8_t,   0x7E0019)   \
    X(over_ent_areas,  uint16_t,  0x1BB96F)   \
    X(over_ent_map16s, uint16_t,  0x1BBA71)   \
    X(over_ent_ids,    uint8_t,   0x1BBB73)   \
    X(over_hle_map16s, uint16_t,  0x1BB800)   \
    X(over_hle_areas,  uint16_t,  0x1BB826)   \
    X(over_hle_ids,    uint8_t,   0x1BB84C)   \

static struct {
#define X(name, type, offset) const type * name;
ALTTP_RAM_LIST
#undef X
} ram;

static struct ap_snes9x * emu = NULL;
char info_string[256];
static struct xy ap_targets[2048];
static size_t ap_target_count = 0;
bool ap_debug;

void ap_init(struct ap_snes9x * _emu) {
    emu = _emu;

#define X(name, type, offset) ram.name = (type *) emu->base(offset);
ALTTP_RAM_LIST
#undef X

    ap_targets[3] = XY(0x928, 0x2199);
    ap_targets[2] = XY(0x928, 0x2150);
    ap_targets[1] = XY(0x978, 0x21c8);
    ap_targets[0] = XY(0x978, 0x21e0);
    ap_target_count = 0;

    printf("----- initialized ------ \n");
    emu->load("last");
}

struct xy
ap_link_xy() {
    return (struct xy) {
        .x = *ram.link_x,
        .y = (uint16_t) (*ram.link_y + 0),
    };
}

void
ap_map_bounds(struct xy * topleft, struct xy * bottomright) {
    if (*ram.in_building) {
        struct xy link = ap_link_xy();
        topleft->x = (uint16_t) (link.x & ~511);
        topleft->y = (uint16_t) (link.y & ~511);
        bottomright->x = (uint16_t) (topleft->x | 511);
        bottomright->y = (uint16_t) (topleft->y | 511);
    } else {
        topleft->x = (uint16_t) (*ram.map_x_offset * 8);
        topleft->y = (uint16_t) (*ram.map_y_offset);
        bottomright->x = (uint16_t) (topleft->x + ((*ram.map_x_mask * 8) | 0xF));
        bottomright->y = (uint16_t) (topleft->y + (*ram.map_y_mask | 0xF));
    }
}

uint16_t
ap_map_attr(struct xy point) {
    if (*ram.in_building) {
        return ram.dngn_bg2_tattr[XYMAP8(point)];
    } else {
#define LOAD(x) (*(uint16_t *)emu->base((x) | 0x7E0000))
#define LOAD2(x) (*(uint16_t *)emu->base((x) ))
        //uint16_t six = ((point.y - LOAD(0x708)) & LOAD(0x70A)) << 3;
        //uint16_t x = (((point.x/8) - LOAD(0x70C)) & LOAD(0x70E)) | six;
        uint16_t a, x, m06;

//CODE_008830:        A5 00         LDA $00                   ;
        a = point.y;
//CODE_008832:        38            SEC                       ;
//CODE_008833:        ED 08 07      SBC $0708                 ;
        a -= LOAD(0x708);
//CODE_008836:        2D 0A 07      AND $070A                 ;
        a &= LOAD(0x70A);
//CODE_008839:        0A            ASL A                     ;
//CODE_00883A:        0A            ASL A                     ;
//CODE_00883B:        0A            ASL A                     ;
        a <<= 3;
//CODE_00883C:        85 06         STA $06                   ;
        m06 = a;
//CODE_00883E:        A5 02         LDA $02                   ;
        a = point.x / 8;
//CODE_008840:        38            SEC                       ;
//CODE_008841:        ED 0C 07      SBC $070C                 ;
        a -= LOAD(0x70C);
//CODE_008844:        2D 0E 07      AND $070E                 ;
        a &= LOAD(0x70E);
//CODE_008847:        05 06         ORA $06                   ;
        a |= m06;
//CODE_008849:        AA            TAX                       ;
        x = a;
//CODE_00884A:        BF 00 20 7E   LDA $7E2000,x             ;
        a = LOAD2(0x7E2000 + x);
//CODE_00884E:        0A            ASL A                     ;
//CODE_00884F:        0A            ASL A                     ;
        a <<= 2;
//CODE_008850:        85 06         STA $06                   ;
        m06 = a;
//CODE_008852:        A5 00         LDA $00                   ;
        a = point.y;
//CODE_008854:        29 08 00      AND #$0008                ;
        a &= 0x8;
//CODE_008857:        4A            LSR A                     ;
//CODE_008858:        4A            LSR A                     ;
        a >>= 2;
//CODE_008859:        04 06         TSB $06                   ;
        m06 |= a;
//CODE_00885B:        A5 02         LDA $02                   ;
        a = point.x / 8;
//CODE_00885D:        29 01 00      AND #$0001                ;
        a &= 0x1;
//CODE_008860:        05 06         ORA $06                   ;
        a |= m06;
//CODE_008862:        0A            ASL A                     ;
        a <<= 1;
//CODE_008863:        AA            TAX                       ;
        x = a;
//CODE_008864:        BF 00 80 0F   LDA.l DATA_0F8000,x       ;
        a = LOAD2(0xF8000 + x);
//CODE_008868:        85 06         STA $06                   ;
        m06 = a;
//CODE_00886A:        29 FF 01      AND #$01FF                ;
        a &= 0x1ff;
//CODE_00886D:        AA            TAX                       ;
        x = a;
//CODE_00886E:        BF 59 94 0E   LDA.l DATA_0E9459,x       ;
        a = LOAD2(0xFFD94 + x);
//CODE_008872:        E2 30         SEP #$30                  ;
        a &= 0xFF;
//CODE_008874:        C9 10         CMP #$10                  ;
//CODE_008876:        90 0F         BCC CODE_008887           ;
//CODE_008878:        C9 1C         CMP #$1C                  ;
//CODE_00887A:        B0 0B         BCS CODE_008887           ;
        if (a < 0x10 || a >= 0x1C) return a;
//CODE_00887C:        85 06         STA $06                   ;
        m06 = (m06 & 0xFF00) | (a & 0xFF);
//CODE_00887E:        A5 07         LDA $07                   ;
        a = (m06 & 0xFF) >> 8;
//CODE_008880:        29 40         AND #$40                  ;
        a &= 0x40;
//CODE_008882:        0A            ASL A                     ;
//CODE_008883:        2A            ROL A                     ;
//CODE_008884:        2A            ROL A                     ;
        a <<= 3;
        a = (a & 0xF8) | ((a >> 8) & 0x7); // XXX is this right w/ carry?
//CODE_008885:        05 06         ORA $06                   ;
        a |= m06 & 0xFF;
//CODE_008887:        6B            RTL                       ;
        return a;

/*
        uint16_t cy = (uint16_t) (point.y - *ram.map_y_offset) & *ram.map_y_mask;
        uint16_t cx = (uint16_t) ((point.x / 8) - *ram.map_x_offset) & *ram.map_x_mask;
        uint16_t offset = (uint16_t) (cy << 3) | cx;
        uint16_t map16 = (uint16_t) (ram.over_map16[offset/2] << 2);
        uint16_t q = (uint16_t) (((point.y & 0x8) >> 2) | ((point.x & 0x8) >> 3));
        uint16_t x = (uint16_t) (map16 | q);
        uint16_t af = ((uint16_t *) emu->base(0x0F8000))[x];
        uint8_t a = ram.over_tattr[af & 0x1ff];
        if (a < 0x10 || a > 0x1C) return a;
        return 0xff;
        */
    }
}
struct xy
ap_map16_to_xy(struct xy tl, uint16_t map16) {
    struct xy xy;
    xy.x = tl.x | ((map16 & 0x03F) << 4);
    xy.y = tl.y | ((map16 & 0xFC0) >> 3);
    return xy;
}

void
ap_print_map() {
    struct xy xy, topleft, bottomright;
    ap_map_bounds(&topleft, &bottomright);
    struct xy link = ap_link_xy();
    for (xy.y = topleft.y; xy.y < bottomright.y; xy.y += 8) {
        for (xy.x = topleft.x; xy.x < bottomright.x; xy.x += 8) {
            if (xy.x >= link.x &&
                xy.x < link.x + 16 &&
                xy.y >= link.y + 8 &&
                xy.y < link.y + 24) {
                printf(TERM_BOLD("%02x "), ap_map_attr(xy));
            } else {
                printf("%02x ", ap_map_attr(xy));
            }
        }
        printf("\n");
    }
    printf("--\n");
    printf(PRIXY " x " PRIXY "\n", PRIXYF(topleft), PRIXYF(bottomright));
}


int
ap_follow_targets(uint16_t * joypad) {
    struct xy link = ap_link_xy();
    struct xy target;
    while (true) {
        if (ap_target_count < 1)
            return 0;

        target = ap_targets[ap_target_count-1];
        if (XYL1DIST(target, link) != 0)
            break;

        ap_target_count--;
    }

    if (link.x > target.x)
        JOYPAD_SET(LEFT);
    else if (link.x < target.x)
        JOYPAD_SET(RIGHT);
    if (link.y > target.y)
        JOYPAD_SET(UP);
    else if (link.y < target.y)
        JOYPAD_SET(DOWN);

    return 1;
}

uint32_t
ap_path_heuristic(struct xy src, struct xy dst) {
    //struct xy vec = XY(ABSDIFF(src.x, dst.x), ABSDIFF(src.y, dst.y));
    return XYL1DIST(src, dst);
}

int
ap_pathfind_local(struct xy destination, bool commit) {
    ap_target_count = 0;

    struct xy link = ap_link_xy();

    struct xy topleft, bottomright;
    ap_map_bounds(&topleft, &bottomright);
    if (destination.x < topleft.x ||
        destination.x > bottomright.x ||
        destination.y < topleft.y ||
        destination.y > bottomright.y) {
        printf("error: out of map\n");
        return -1;
    }

    struct xy size = XYOP2(bottomright, -, topleft);
    struct xy grid = XYOP1(size, / 8);
    struct xy tl_offset = XYOP1(topleft, / 8);

    struct xy src = XYOP2(XYOP1(link, / 8), -, tl_offset);
    struct xy dst = XYOP2(XYOP1(destination, / 8), -, tl_offset);

    uint32_t distance = ap_path_heuristic(src, dst);
    if (distance > (size.x + size.y)) {
        printf("error: invalid target " PRIXY, PRIXYF(destination));
        return -1;
    }
    if (distance < 8) {
        ap_targets[0] = destination;
        return ap_target_count = 1;
    }

    static struct point_state {
        uint8_t dirfrom;
        uint32_t gscore;
        uint32_t fscore;
        uint32_t cost;
    } buf[0x82 * 0x82];  // xxx: can this be 0x81 x 0x81?
    if (grid.x * grid.y > 0x80 * 0x80) {
        printf("error: grid too large: " PRIXY "\n", PRIXYF(grid));
        return -1;
    }
    // Make a state[y][x]
    struct point_state (*state)[grid.x] = (void *) &buf[0x101];

    for (uint16_t y = 0; y < grid.y; y++) {
        for (uint16_t x = 0; x < grid.x; x++) {
            state[y][x].dirfrom = 0;
            state[y][x].gscore = (uint32_t) -1;
            state[y][x].fscore = (uint32_t) -1;
            state[y][x].cost = 0;
        }
    }
    for (uint16_t y = 0; y < grid.y; y++) {
        for (uint16_t x = 0; x < grid.x; x++) {
            uint32_t cost = 0;
            struct xy mapxy = XYOP2(XYOP1(XY(x, y), * 8), +, topleft);
            uint8_t tile = alttp_tile_attrs[ap_map_attr(mapxy)];
            if (tile & (TILE_ATTR_WALK | TILE_ATTR_DOOR))
                cost = 0;
            else if (tile & TILE_ATTR_LFT0)
                cost = 1000;
            else
                cost = (1 << 20);

            // Link's XY cordinate is for his hat, his collision is lower
            state[y-1][x-0].cost += cost;
            state[y-1][x-1].cost += cost;
            state[y-2][x-0].cost += cost;
            state[y-2][x-1].cost += cost;
        }
    }

    static struct pq * pq = NULL;
    if (pq == NULL) {
        pq = pq_create(sizeof(struct xy));
        if (pq == NULL) exit(1);
    }
    pq_clear(pq);
    pq_push(pq, 0, &src);

    state[src.y][src.x].dirfrom = 9;
    state[src.y][src.x].fscore = 0;
    state[src.y][src.x].gscore = 0;
    
    size_t r;
    uint64_t min_heuristic = -1;
    for (r = 0; r < 0x80 * 0x80 + 100; r++) {
        struct xy node;
        uint64_t cost;
        int rc = pq_pop(pq, &cost, &node);
        if (rc != 0)
            goto search_failed;
        if (cost != state[node.y][node.x].fscore)
            continue;
        for (uint8_t i = 1; i < 9; i++) {
            struct xy neighbor = {
                .x = (uint16_t) (node.x + dir_dx[i]),
                .y = (uint16_t) (node.y + dir_dy[i]),
            };
            if (neighbor.x >= grid.x || neighbor.y >= grid.y)
                continue;

            uint32_t gscore = state[node.y][node.x].gscore;
            gscore += dir_cost[i];
            gscore += state[neighbor.y][neighbor.x].cost;
            if (gscore >= (1 << 20))
                continue;
            if (gscore >= state[neighbor.y][neighbor.x].gscore)
                continue;

            uint32_t heuristic = ap_path_heuristic(neighbor, dst);
            min_heuristic = MIN(heuristic, min_heuristic);
            uint32_t fscore = gscore + heuristic;
            state[neighbor.y][neighbor.x].dirfrom = i;
            state[neighbor.y][neighbor.x].gscore = gscore;
            state[neighbor.y][neighbor.x].fscore = fscore;
            
            if (neighbor.x == dst.x && neighbor.y == dst.y) {
                goto search_done;
            }
            pq_push(pq, fscore, &neighbor);
        }
    }
    if (commit) printf("A* timed out, min heuristic: %lu\n", min_heuristic);
    return -1;
search_failed:
    if (commit) printf("A* failed, min heuristic: %lu\n", min_heuristic);
    return -1;
search_done:
    if (commit) printf("A* done in %zu steps, pq size = %zu\n", r, pq_size(pq));
    else return 1;

    ap_target_count = 0;
    ap_targets[ap_target_count++] = destination;

    struct xy next = dst;
    uint8_t last_dirfrom = -1;
    while (!(next.x == src.x && next.y == src.y)) {
        uint8_t i = state[next.y][next.x].dirfrom;
        next.x = (uint16_t) (next.x - dir_dx[i]);
        next.y = (uint16_t) (next.y - dir_dy[i]);

        if (i != last_dirfrom) {
            ap_targets[ap_target_count++] = XYOP1(XYOP2(next, +, tl_offset), * 8);
            printf("%zu, %u: "PRIXY "\n", ap_target_count, i, PRIXYF(ap_targets[ap_target_count-1]));
        }
        last_dirfrom = i;
    }

    return ap_target_count;
}

void
ap_update_map_node() {
    struct xy tl, br;
    ap_map_bounds(&tl, &br);
    size_t index = XYMAPNODE(tl);
    if (index == last_map_node_idx)
        return;

    struct ap_node *map_node = &map_nodes[index];
    if (map_node->node_parent != NULL) {
        // TODO: support updating
        assert(map_node->node_parent == map_node);
        return;
    }

    struct xy xy;
    for (xy.y = tl.y; xy.y < br.y; xy.y += 0x100) {
        for (xy.x = tl.x; xy.x < br.x; xy.x += 0x100) {
            map_nodes[XYMAPNODE(xy)].node_parent = map_node;
        }
    }
    map_node->node_next = map_node;
    map_node->node_prev = map_node;
    map_node->tl = tl;
    map_node->br = br;
    snprintf(map_node->name, sizeof map_node->name, "map 0x%02zX", index);

    int node_count = 0;
    struct ap_node * new_node = NULL;
    uint16_t walk_mask = TILE_ATTR_WALK | TILE_ATTR_DOOR;
    for (xy = tl; xy.x < br.x; xy.x += 8) { // TL to TR
        if (alttp_tile_attrs[ap_map_attr(xy)] & walk_mask) {
            if (new_node == NULL) {
                new_node = ap_node_append(map_node);
                new_node->tl = xy;
                snprintf(new_node->name, sizeof new_node->name, "N%d", ++node_count);
            }
            new_node->br = xy;
        } else if (new_node != NULL) {
            new_node = NULL;
        }
    }
    new_node = NULL;
    for (xy.x = br.x - 8; xy.y < br.y; xy.y += 8) { // TR to BR
        if (alttp_tile_attrs[ap_map_attr(xy)] & walk_mask) {
            if (new_node == NULL) {
                new_node = ap_node_append(map_node);
                new_node->tl = xy;
                snprintf(new_node->name, sizeof new_node->name, "E%d", ++node_count);
            }
            new_node->br = xy;
        } else if (new_node != NULL) {
            new_node = NULL;
        }
    }
    new_node = NULL;
    for (xy = tl; xy.y < br.y; xy.y += 8) { // TL to BL
        if (alttp_tile_attrs[ap_map_attr(xy)] & walk_mask) {
            if (new_node == NULL) {
                new_node = ap_node_append(map_node);
                new_node->tl = xy;
                snprintf(new_node->name, sizeof new_node->name, "W%d", ++node_count);
            }
            new_node->br = xy;
        } else if (new_node != NULL) {
            new_node = NULL;
        }
    }
    new_node = NULL;
    for (xy.y = br.y - 8; xy.x < br.x; xy.x += 8) { // BL to BR
        if (alttp_tile_attrs[ap_map_attr(xy)] & walk_mask) {
            if (new_node == NULL) {
                new_node = ap_node_append(map_node);
                new_node->tl = xy;
                snprintf(new_node->name, sizeof new_node->name, "S%d", ++node_count);
            }
            new_node->br = xy;
        } else if (new_node != NULL) {
            new_node = NULL;
        }
    }
    if (!*ram.in_building) {
        for (size_t i = 0; i < 0x81; i++) {
            if (ram.over_ent_areas[i] != *ram.map_area)
                continue;
            new_node = ap_node_append(map_node);
            node_count++;
            new_node->tl = ap_map16_to_xy(tl, ram.over_ent_map16s[i]);
            new_node->br = XYOP1(new_node->tl, + 15);
            snprintf(new_node->name, sizeof new_node->name, "door 0x%02x", ram.over_ent_ids[i]);
        }
        /* TODO: work out map16 decoding
        for (size_t i = 1; i < 0x1C; i++) {
            if (ram.over_hle_areas[i] != *ram.map_area)
                continue;
            new_node = ap_node_append(map_node);
            node_count++;
            new_node->tl = ap_map16_to_xy(tl, ram.over_hle_map16s[i]);
            new_node->br = XYOP1(new_node->tl, + 15);
            snprintf(new_node->name, sizeof new_node->name, "hole 0x%02x", ram.over_hle_ids[i]);
        }
        */
    }
    for (xy.y = tl.y; xy.y < br.y; xy.y += 0x10) {
        for (xy.x = tl.x; xy.x < br.x; xy.x += 0x10) {
            uint8_t attr = ap_map_attr(xy);
            if ((alttp_tile_attrs[attr] & TILE_ATTR_ITEM) || attr == 0x20) {
                new_node = ap_node_append(map_node);
                node_count++;
                snprintf(new_node->name, sizeof new_node->name, "item 0x%02x", attr);
                if (ap_map_attr(XYOP1(xy, + 15)) == attr) {
                    new_node->tl = xy;
                    new_node->br = XYOP1(xy, + 15);
                } else if (ap_map_attr(XYOP1(xy, - 8)) == attr) {
                    new_node->tl = XYOP1(xy, - 8);
                    new_node->br = XYOP1(xy, + 7);
                }
            }
        }
    }

    ap_print_map();
    printf("found %d nodes\n", node_count);
    new_node = map_node;
    int i = 0; 
    do {
        bool reachable = ap_pathfind_local(new_node->tl, false) > 0;
        printf("   %d. [%c] (" PRIXY " x " PRIXY ") %s\n", i++, "!."[reachable], PRIXYF(new_node->tl), PRIXYF(new_node->br), new_node->name);
        new_node = new_node->node_next;
    } while(new_node != map_node);

    while (true) {
        printf("Choose 1-%d>", node_count);
        fflush(NULL);
        fseek(stdin,0,SEEK_END);
        if (scanf("%d", &i) < 1 || i > 100)
            continue;
        if (i == 0) break;
        new_node = map_node;
        while (i--) {
            new_node = new_node->node_next;
        }
        printf("\nGoing to: %s\n", new_node->name);
        //ap_pathfind_local(XYMID(new_node->tl, new_node->br));
        int rc = ap_pathfind_local(new_node->tl, true);
        if (rc >= 0) break;
    }
}

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

void
ap_tick(uint32_t frame, uint16_t * joypad) {
    *emu->info_string_ptr = info_string;

    if (JOYPAD_EVENT(START)) {
        JOYPAD_CLEAR(START);
        if (frame > 60)
            emu->save("last");
    }

    struct xy topleft, bottomright;
    ap_map_bounds(&topleft, &bottomright);
    struct xy link = ap_link_xy();

    static uint16_t last_map = -1; 
    static uint16_t transition_counter = 0;
    if (last_map == XYMAPNODE(topleft)) {
        transition_counter++;
    } else {
        last_map = XYMAPNODE(topleft);
        transition_counter = 0;
    }
    if (transition_counter < 128 || !XYIN(link, topleft, bottomright)) {
        JOYPAD_SET(B);
        return;
    }


    ap_update_map_node();
    ap_debug = JOYPAD_TEST(START);

    JOYPAD_CLEAR(START);

    if (!JOYPAD_TEST(TR)) {
        ap_follow_targets(joypad);
    }
    if (JOYPAD_EVENT(TR)) {
        int rc = ap_pathfind_local(XY(0x940, 0xA20), true);
        printf("pathfind result: %d\n", rc);
    }

    //INFO_HEXDUMP(ram.map_area);

    //INFO("L:" PRIXY " M: " PRIXY "," PRIXY, PRIXYF(ap_link_xy()), PRIXYF(topleft), PRIXYF(bottomright));

    INFO("L:" PRIXY "; %zu:" PRIXY, PRIXYF(link), ap_target_count, PRIXYF(ap_targets[ap_target_count-1]));

    if (JOYPAD_EVENT(Y)) {
        ap_print_map();
    }



    //uint8_t *b = emu->base(0x7E001A);
    //INFO_HEXDUMP(b);
    if (frame % 2) {
        JOYPAD_SET(A);
    }
}
