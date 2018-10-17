#include "ap_map.h"
#include "ap_math.h"
#include "ap_macro.h"
#include "ap_snes.h"
#include "ap_plan.h"
#include "pq.h"

static struct ap_node map_nodes[0x100 * 0x100];
static size_t last_map_node_idx = -1;

static struct xy ap_targets[2048];
static size_t ap_target_count = 0;
static int ap_target_timeout = 0;

static struct ap_node *
ap_node_append(struct ap_node * parent)
{
    struct ap_node * child = calloc(1, sizeof *child);
    assert(child != NULL);

    child->node_parent = parent;
    LL_PUSH(parent, child);
    return child;
}

struct xy
ap_link_xy()
{
    // Returns the effective tl coordinate that link interacts with
    // Which is 8 pixels below his head
    return (struct xy) {
        .x = *ap_ram.link_x,
        .y = (uint16_t) (*ap_ram.link_y + 8),
    };
}

void
ap_map_bounds(struct xy * topleft, struct xy * bottomright)
{
    if (*ap_ram.in_building) {
        struct xy link = ap_link_xy();
        link.y = CAST16(link.y - 8);
        topleft->x = (uint16_t) (link.x & ~511);
        topleft->y = (uint16_t) (link.y & ~511);
        bottomright->x = (uint16_t) (topleft->x | 511);
        bottomright->y = (uint16_t) (topleft->y | 511);
    } else {
        topleft->x = (uint16_t) (*ap_ram.map_x_offset * 8);
        topleft->y = (uint16_t) (*ap_ram.map_y_offset);
        bottomright->x = (uint16_t) (topleft->x + ((*ap_ram.map_x_mask * 8) | 0xF));
        bottomright->y = (uint16_t) (topleft->y + (*ap_ram.map_y_mask | 0xF));
    }
}

static uint16_t
ap_map_attr(struct xy point)
{
    if (*ap_ram.in_building) {
        return ap_ram.dngn_bg2_tattr[XYMAP8(point)];
    } else {
#define LOAD(x) (*(uint16_t *)ap_emu->base((x) | 0x7E0000))
#define LOAD2(x) (*(uint16_t *)ap_emu->base((x) ))
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
        uint16_t cy = (uint16_t) (point.y - *ap_ram.map_y_offset) & *ap_ram.map_y_mask;
        uint16_t cx = (uint16_t) ((point.x / 8) - *ap_ram.map_x_offset) & *ap_ram.map_x_mask;
        uint16_t offset = (uint16_t) (cy << 3) | cx;
        uint16_t map16 = (uint16_t) (ap_ram.over_map16[offset/2] << 2);
        uint16_t q = (uint16_t) (((point.y & 0x8) >> 2) | ((point.x & 0x8) >> 3));
        uint16_t x = (uint16_t) (map16 | q);
        uint16_t af = ((uint16_t *) ap_emu->base(0x0F8000))[x];
        uint8_t a = ap_ram.over_tattr[af & 0x1ff];
        if (a < 0x10 || a > 0x1C) return a;
        return 0xff;
        */
    }
}

static struct xy
ap_map16_to_xy(struct xy tl, uint16_t map16)
{
    struct xy xy;
    //xy.x = tl.x + ((map16 & 0x03F) << 4);
    //xy.y = tl.y + ((map16 & 0xFC0) >> 2);
    xy.x = (*ap_ram.map_x_offset * 8) + ((map16 & 0x07E) << 3) + 8;
    xy.y = *ap_ram.map_y_offset + ((map16 & 0x1F80) >> 3) + 16;
    return xy;
}

static struct ap_node *
ap_map_node(struct xy tl);

void
ap_print_map()
{
    FILE * mapf = fopen("map", "w");
    struct xy xy, topleft, bottomright;
    ap_map_bounds(&topleft, &bottomright);
    struct xy link = ap_link_xy();
    //struct xy link_tl = XY(link.x, link.y + 8);
    //struct xy link_br = XY(link.x + 15, link.y + 23);
    struct xy link_tl = link;
    struct xy link_br = XYOP1(link, + 15);
    struct ap_node * map_node = ap_map_node(topleft);

    for (xy.y = topleft.y; xy.y < bottomright.y; xy.y += 8) {
        for (xy.x = topleft.x; xy.x < bottomright.x; xy.x += 8) {
            uint8_t tile_attr = ap_map_attr(xy);
            if (XYIN(xy, link_tl, link_br)) {
                fprintf(mapf, TERM_GREEN(TERM_BOLD("%02x ")), tile_attr);
                goto next_point;
            } 
            if (map_node != NULL) {
                for (struct ap_node * node = map_node->next; node != map_node; node = node->next) {
                    if (XYIN(xy, node->tl, node->br)) {
                        fprintf(mapf, TERM_RED(TERM_BOLD("%02x ")), tile_attr);
                        goto next_point;
                    }
                }
            }
            for (size_t i = 0; i < ap_target_count; i++) {
                if (XYIN(xy, ap_targets[i], XYOP1(ap_targets[i], +8))) {
                    fprintf(mapf, TERM_BLUE(TERM_BOLD("%02x ")), tile_attr);
                    goto next_point;
                }
            }
            if (!(ap_tile_attrs[tile_attr] & TILE_ATTR_WALK)) {
                fprintf(mapf, TERM_BOLD("%02x "), ap_map_attr(xy));
                goto next_point;
            }
            fprintf(mapf, "%02x ", ap_map_attr(xy));
next_point:;
        }
        fprintf(mapf, "\n");
    }
    fprintf(mapf, "--\n");
    fprintf(mapf, PRIXY " x " PRIXY "\n", PRIXYF(topleft), PRIXYF(bottomright));
    fflush(mapf);
}

static int
ap_set_targets(size_t count)
{
    assert(count < sizeof(ap_targets) / sizeof(*ap_targets));
    ap_target_count = count;
    ap_target_timeout = 0;
    if (count == 0) 
        return 0;

    ap_targets[count] = ap_link_xy();
    for (size_t i = 0; i < count; i++) {
        ap_target_timeout += XYL1DIST(ap_targets[i], ap_targets[i+1]) * 2;
    }

    ap_target_timeout += 64;
    return ap_target_timeout;
}

int
ap_follow_targets(uint16_t * joypad)
{
    struct xy link = ap_link_xy();
    if (ap_target_timeout <= 0) {
        INFO("L:" PRIXY "; " PRIXY " timeout", PRIXYF(link), PRIXYF(ap_targets[0]));
        LOG("L:" PRIXY "; " PRIXY " timeout", PRIXYF(link), PRIXYF(ap_targets[0]));
        ap_target_count = 0;
        return RC_FAIL;
    }

    struct xy target;
    while (true) {
        if (ap_target_count < 1) {
            INFO("L:" PRIXY "; done", PRIXYF(link));
            LOG("L:" PRIXY "; done", PRIXYF(link));
            return RC_DONE;
        }

        target = ap_targets[ap_target_count-1];
        if (XYL1DIST(target, link) > 0)
            break;

        ap_target_count--;
    }

    INFO("L:" PRIXY "; %zu:" PRIXY "; %d", PRIXYF(link), ap_target_count, PRIXYF(ap_targets[ap_target_count-1]), ap_target_timeout);
    LOG("L:" PRIXY "; %zu:" PRIXY "; %d", PRIXYF(link), ap_target_count, PRIXYF(ap_targets[ap_target_count-1]), ap_target_timeout);

    if (link.x > target.x)
        JOYPAD_SET(LEFT);
    else if (link.x < target.x)
        JOYPAD_SET(RIGHT);
    if (link.y > target.y)
        JOYPAD_SET(UP);
    else if (link.y < target.y)
        JOYPAD_SET(DOWN);

    ap_target_timeout--;

    return RC_INPR;
}

void
ap_joypad_setdir(uint16_t * joypad, uint8_t dir)
{
    // 0 U D L R LU LD RU RD 0
    JOYPAD_CLEAR(UP);
    JOYPAD_CLEAR(DOWN);
    JOYPAD_CLEAR(LEFT);
    JOYPAD_CLEAR(RIGHT);
    switch (dir) {
    case 1: JOYPAD_SET(UP); break;
    case 2: JOYPAD_SET(DOWN); break;
    case 3: JOYPAD_SET(LEFT); break;
    case 4: JOYPAD_SET(RIGHT); break;
    case 5: JOYPAD_SET(UP); JOYPAD_SET(LEFT); break;
    case 6: JOYPAD_SET(UP); JOYPAD_SET(LEFT); break;
    case 7: JOYPAD_SET(DOWN); JOYPAD_SET(RIGHT); break;
    case 8: JOYPAD_SET(DOWN); JOYPAD_SET(RIGHT); break;
    }
}

uint32_t
ap_path_heuristic(struct xy src, struct xy dst_tl, struct xy dst_br)
{
    uint32_t distance = 0;
    if (src.x < dst_tl.x)
        distance += dst_tl.x - src.x;
    else if (src.x > dst_br.x)
        distance += src.x - dst_br.x;
    if (src.y < dst_tl.y)
        distance += dst_tl.y - src.y;
    else if (src.y > dst_br.y)
        distance += src.y - dst_br.y;
    return distance;
}

static int
ap_pathfind_local(struct xy destination_tl, struct xy destination_br, bool commit)
{
    ap_set_targets(0);

    struct xy link = ap_link_xy();

    struct xy topleft, bottomright;
    ap_map_bounds(&topleft, &bottomright);
    if (!XYIN(destination_tl, topleft, bottomright)) {
        LOG("error: TL destination " PRIXYV "out of map", PRIXYVF(destination_tl));
        return -1;
    }
    if (!XYIN(destination_br, topleft, bottomright)) {
        LOG("error: BR destination " PRIXYV "out of map", PRIXYVF(destination_br));
        return -1;
    }

    struct xy size = XYOP2(bottomright, -, topleft);
    struct xy grid = XYOP1(size, / 8);
    struct xy tl_offset = XYOP1(topleft, / 8);

    struct xy src = XYOP2(XYOP1(link, / 8), -, tl_offset);
    struct xy dst_tl = XYOP2(XYOP1(destination_tl, / 8), -, tl_offset);
    struct xy dst_br = XYOP2(XYOP1(destination_br, / 8), -, tl_offset);

    uint32_t distance = ap_path_heuristic(src, dst_tl, dst_br);
    if (distance > (size.x + size.y)) {
        printf("error: invalid target " PRIXYV " x " PRIXYV, PRIXYVF(destination_tl), PRIXYVF(destination_br));
        return -1;
    }
    /*
    if (distance < 8) {
        ap_targets[0] = destination;
        return ap_target_count = 1;
    }
    */

    struct point_state;
    static struct point_state {
        struct point_state * from;
        struct xy xy;
        uint8_t tile_attrs;
        uint8_t ledge;
        uint32_t gscore;
        uint32_t fscore;
        uint32_t cost;
    } buf[0x82 * 0x82]; 
    if (grid.x * grid.y > 0x80 * 0x80) {
        printf("error: grid too large: " PRIXY "\n", PRIXYF(grid));
        return -1;
    }
    // Make a state[y][x]
    struct point_state (*state)[grid.x] = (void *) &buf[0x101];

    for (uint16_t y = 0; y < grid.y; y++) {
        for (uint16_t x = 0; x < grid.x; x++) {
            state[y][x].from = NULL;
            state[y][x].xy = XYOP1(XYOP2(XY(x, y), +, tl_offset), * 8);
            state[y][x].gscore = (uint32_t) -1;
            state[y][x].fscore = (uint32_t) -1;
            state[y][x].cost = 0;
            state[y][x].ledge = 0;
            state[y][x].tile_attrs = 0;
        }
    }
    for (uint16_t y = 0; y < grid.y; y++) {
        for (uint16_t x = 0; x < grid.x; x++) {
            uint32_t cost = 0;
            struct xy mapxy = XYOP2(XYOP1(XY(x, y), * 8), +, topleft);
            uint8_t tile = ap_tile_attrs[ap_map_attr(mapxy)];
            state[y][x].tile_attrs = tile;
            if (tile & (TILE_ATTR_WALK | TILE_ATTR_DOOR)) {
                cost = 0;
            } else if (tile & TILE_ATTR_LDGE) {
                cost = (1 << 20); // calculated at search time
                state[y][x].ledge = (tile & 0x7) + 1;
            } else if (tile & TILE_ATTR_LFT0) {
                cost = 5000;
            } else if (XYIN(mapxy, dst_tl, dst_br)) {
                cost = 0;
            } else {
                cost = (1 << 20);
            }

            state[y-0][x-0].cost += cost;
            state[y-0][x-1].cost += cost;
            state[y-1][x-0].cost += cost;
            state[y-1][x-1].cost += cost;
        }
    }

    static struct pq * pq = NULL;
    if (pq == NULL) {
        pq = pq_create(sizeof(struct xy));
        if (pq == NULL) exit(1);
    }
    pq_clear(pq);
    pq_push(pq, 0, &src);

    state[src.y][src.x].fscore = 0;
    state[src.y][src.x].gscore = 0;
    
    size_t r;
    struct xy final_xy;
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

            // Only pick up objects if you are square with them
            if (i == DIR_U || i == DIR_D || i >= 5) {
                if ((state[neighbor.y][node.x].tile_attrs & TILE_ATTR_LFT0) !=
                    (state[neighbor.y][node.x+1].tile_attrs & TILE_ATTR_LFT0))
                    continue;
            }
            if (i == DIR_L || i == DIR_R || i >= 5) {
                if ((state[node.y][neighbor.x].tile_attrs & TILE_ATTR_LFT0) !=
                    (state[node.y+1][neighbor.x].tile_attrs & TILE_ATTR_LFT0))
                    continue;
            }
            // Jump down ledges
            if (state[node.y][node.x].ledge == i) {
                while (neighbor.x < grid.x && neighbor.y < grid.y && neighbor.x > 0 && neighbor.y > 0) {
                    if (state[neighbor.y][neighbor.x].cost < (1 << 20))
                        break;
                    neighbor.x += dir_dx[i];
                    neighbor.y += dir_dy[i];
                }
            }

            uint32_t gscore = state[node.y][node.x].gscore;
            gscore += dir_cost[i];
            if (!state[neighbor.y][neighbor.x].ledge)
                gscore += state[neighbor.y][neighbor.x].cost;
            if (gscore >= (1 << 20))
                continue;
            if (gscore >= state[neighbor.y][neighbor.x].gscore)
                continue;

            uint32_t heuristic = ap_path_heuristic(neighbor, dst_tl, dst_br);
            if (heuristic == 0) {
                final_xy = neighbor;
                state[neighbor.y][neighbor.x].from = &state[node.y][node.x];
                goto search_done;
            }

            min_heuristic = MIN(heuristic, min_heuristic);
            uint32_t fscore = gscore + heuristic;
            state[neighbor.y][neighbor.x].from = &state[node.y][node.x];
            state[neighbor.y][neighbor.x].gscore = gscore;
            state[neighbor.y][neighbor.x].fscore = fscore;
            
            pq_push(pq, fscore, &neighbor);
        }
    }
    if (commit) LOG("A* timed out, min heuristic: %lu", min_heuristic);
    return -1;
search_failed:
    if (commit) LOG("A* failed, min heuristic: %lu", min_heuristic);
    return -1;
search_done:
    if (commit) LOG("A* done in %zu steps, pq size = %zu", r, pq_size(pq));
    else return 1;

    struct point_state * next = &state[final_xy.y][final_xy.x];
    size_t count = 0;
    ap_targets[count++] = XY(
            MIN(next->xy.x, destination_br.x - 15),
            MIN(next->xy.y, destination_br.y - 15));

    struct xy last_dir = XY(1, 2);
    struct xy last_xy = XY(3, 4);
    while (next != NULL) {
        struct xy dir = XYOP2(next->xy, -, last_xy);
        if (!next->ledge) {
            if (!XYEQ(dir, last_dir)) {
                ap_targets[count++] = next->xy;
                //printf("%zu: "PRIXY "\n", count, PRIXYF(ap_targets[count-1]));
            }
        }
        last_dir = dir;
        last_xy = next->xy;
        next = next->from;
    }

    int timeout = ap_set_targets(count);
    ap_print_map();
    return timeout;
}

int
ap_pathfind_node(struct ap_node * node)
{
    struct xy tl = node->tl;
    struct xy br = node->br;
    if (node->adjacent_direction) {
        struct xy offset = XYOP1(dir_dxy[node->adjacent_direction], * -7);
        tl = XYOP2(tl, +, offset);
        br = XYOP2(br, +, offset);
    }
    return ap_pathfind_local(tl, br, true);
}

static struct ap_node *
ap_map_node(struct xy tl) {
    size_t index = XYMAPNODE(tl);
    struct ap_node * map_node = &map_nodes[index];
    if (map_node->node_parent != NULL)
        return map_node->node_parent;
    return map_node;
}

struct ap_node *
ap_update_map_node()
{
    static bool once = false;
    if (!once) {
        once = true;
        for (int x = 0; x < 0x10000; x += 0x100) {
            for (int y = 0; y < 0x10000; y += 0x100) {
                snprintf(map_nodes[XYMAPNODE(XY(x, y))].name, sizeof map_nodes->name, "umap %02x,%02x", x >> 8, y >> 8);
            }
        }
    }
    struct xy tl, br;
    ap_map_bounds(&tl, &br);
    size_t index = XYMAPNODE(tl);
    struct ap_node *map_node = &map_nodes[index];
    if (index == last_map_node_idx)
        return map_node;
    last_map_node_idx = index;

    if (map_node->node_parent != NULL) {
        // TODO: support graceful updating
        assert(map_node->node_parent == map_node);

        while (true) {
            struct ap_node * node = LL_POP(map_node);
            if (node == NULL)
                break;
            free(node);
        }
    }

    struct xy xy;
    for (xy.y = tl.y; xy.y < br.y; xy.y += 0x100) {
        for (xy.x = tl.x; xy.x < br.x; xy.x += 0x100) {
            map_nodes[XYMAPNODE(xy)].node_parent = map_node;
        }
    }
    map_node->next = map_node;
    map_node->prev = map_node;
    map_node->tl = tl;
    map_node->br = br;
    snprintf(map_node->name, sizeof map_node->name, "0x%02zX map", index);

    int node_count = 0;
    struct ap_node * new_node = NULL;
    uint16_t walk_mask = TILE_ATTR_WALK | TILE_ATTR_DOOR;
    for (xy = tl; xy.x < br.x; xy.x += 8) { // TL to TR
        if (ap_tile_attrs[ap_map_attr(xy)] & walk_mask) {
            if (new_node == NULL) {
                new_node = ap_node_append(map_node);
                new_node->tl = xy;
                new_node->adjacent_direction = DIR_U;
                new_node->node_adjacent = ap_map_node(XY(xy.x, xy.y - 0x100));
                snprintf(new_node->name, sizeof new_node->name, "0x%02zX N%d", index, ++node_count);
            }
            new_node->br = XYOP1(xy, + 7);
            new_node->br.y += 8;
        } else if (new_node != NULL) {
            new_node = NULL;
        }
    }
    new_node = NULL;
    for (xy = XY(br.x & ~7, tl.y); xy.y < br.y; xy.y += 8) { // TR to BR
        if (ap_tile_attrs[ap_map_attr(xy)] & walk_mask) {
            if (new_node == NULL) {
                new_node = ap_node_append(map_node);
                new_node->tl = xy;
                new_node->tl.x -= 8;
                new_node->adjacent_direction = DIR_R;
                new_node->node_adjacent = ap_map_node(XY(xy.x + 0x100, xy.y));
                snprintf(new_node->name, sizeof new_node->name, "0x%02zX E%d", index, ++node_count);
            }
            new_node->br = XYOP1(xy, + 7);
        } else if (new_node != NULL) {
            new_node = NULL;
        }
    }
    new_node = NULL;
    for (xy = tl; xy.y < br.y; xy.y += 8) { // TL to BL
        if (ap_tile_attrs[ap_map_attr(xy)] & walk_mask) {
            if (new_node == NULL) {
                new_node = ap_node_append(map_node);
                new_node->tl = xy;
                new_node->adjacent_direction = DIR_L;
                new_node->node_adjacent = ap_map_node(XY(xy.x - 0x100, xy.y));
                snprintf(new_node->name, sizeof new_node->name, "0x%02zX W%d", index, ++node_count);
            }
            new_node->br = XYOP1(xy, + 7);
            new_node->br.x += 8;
        } else if (new_node != NULL) {
            new_node = NULL;
        }
    }
    new_node = NULL;
    for (xy = XY(tl.x, br.y & ~7); xy.x < br.x; xy.x += 8) { // BL to BR
        if (ap_tile_attrs[ap_map_attr(xy)] & walk_mask) {
            if (new_node == NULL) {
                new_node = ap_node_append(map_node);
                new_node->tl = xy;
                new_node->tl.y -= 8;
                new_node->adjacent_direction = DIR_D;
                new_node->node_adjacent = ap_map_node(XY(xy.x, xy.y + 0x100));
                snprintf(new_node->name, sizeof new_node->name, "0x%02zX S%d", index, ++node_count);
            }
            new_node->br = XYOP1(xy, + 7);
        } else if (new_node != NULL) {
            new_node = NULL;
        }
    }
    if (!*ap_ram.in_building && false) {
        LOG("xo: %04x, xm: %04x, yo: %04x, ym: %04x",
            *ap_ram.map_x_offset, *ap_ram.map_x_mask, *ap_ram.map_y_offset, *ap_ram.map_y_mask);
        for (size_t i = 0; i < 0x81; i++) {
            if (ap_ram.over_ent_areas[i] != *ap_ram.map_area)
                continue;
            new_node = ap_node_append(map_node);
            node_count++;
            new_node->tl = ap_map16_to_xy(tl, ap_ram.over_ent_map16s[i]);
            new_node->br = XYOP1(new_node->tl, + 15);
            new_node->tile_attr = 0x80; // not quite true
            uint16_t id = ap_ram.over_ent_ids[i];
            if (id <= 0x85) {
                new_node->node_adjacent = ap_map_node(XY(ap_ram.entrance_xs[id], ap_ram.entrance_ys[id]));
                new_node->adjacent_direction = DIR_U;
            }
            snprintf(new_node->name, sizeof new_node->name, "door 0x%02x", ap_ram.over_ent_ids[i]);
            LOG("tl: " PRIXYV ", out: " PRIXYV ", map16: %04x", PRIXYVF(tl), PRIXYVF(new_node->tl), ap_ram.over_ent_map16s[i]);
        }
        /* TODO: work out map16 decoding
        for (size_t i = 1; i < 0x1C; i++) {
            if (ap_ram.over_hle_areas[i] != *ap_ram.map_area)
                continue;
            new_node = ap_node_append(map_node);
            node_count++;
            new_node->tl = ap_map16_to_xy(tl, ap_ram.over_hle_map16s[i]);
            new_node->br = XYOP1(new_node->tl, + 15);
            snprintf(new_node->name, sizeof new_node->name, "hole 0x%02x", ap_ram.over_hle_ids[i]);
        }
        */
    }
    for (xy.y = tl.y; xy.y < br.y; xy.y += 0x10) {
        for (xy.x = tl.x; xy.x < br.x; xy.x += 0x10) {
            uint8_t attr = ap_map_attr(xy);
            if (ap_tile_attrs[attr] & TILE_ATTR_ITEM) {
                new_node = ap_node_append(map_node);
                new_node->tile_attr = attr;
                node_count++;
                snprintf(new_node->name, sizeof new_node->name, "item 0x%02x", attr);
                if (ap_map_attr(XYOP1(xy, + 15)) == attr) {
                    new_node->tl = xy;
                    new_node->br = XYOP1(xy, + 15);
                } else if (ap_map_attr(XYOP1(xy, - 8)) == attr) {
                    new_node->tl = XYOP1(xy, - 8);
                    new_node->br = XYOP1(xy, + 7);
                }
                if (ap_tile_attrs[attr] & TILE_ATTR_CHST) {
                    // shift chests down to the square below
                    new_node->tl.y += 16;
                    new_node->br.y += 16;
                }
            }
        }
    }

    for (struct ap_node * node = map_node->next; node != map_node; node = node->next) {
        if (node->node_adjacent) {
            if (node->node_adjacent->node_parent == NULL)
                ap_goal_add(GOAL_EXPLORE, node);
        } else if (node->tile_attr) {
            ap_goal_add(GOAL_ITEM, node);
        }
    }

    ap_print_map();
    printf("found %d nodes\n", node_count);
    int i = 0; 
    for (struct ap_node * node = map_node->next; node != map_node; node = node->next) {
        bool reachable = ap_pathfind_local(node->tl, node->br, false) > 0;
        node->_reachable = reachable;
        printf("   %d. [%c] (" PRIXY " x " PRIXY ") %s\n", i++, "!."[reachable], PRIXYF(node->tl), PRIXYF(node->br), node->name);
    }

    /*
    for (int tries = 100; tries; tries--) {
        printf("Choose 1-%d>", node_count);
        fflush(NULL);
        fseek(stdin,0,SEEK_END);
        if (scanf("%d", &i) < 1 || i > 100)
            continue;
        if (i == 0) break;
        new_node = map_node;
        while (i--) {
            new_node = new_node->next;
        }
        printf("\nGoing to: %s\n", new_node->name);
        //ap_pathfind_local(XYMID(new_node->tl, new_node->br));
        int rc = ap_pathfind_local(new_node->tl, new_node->br, true);
        if (rc >= 0) break;
    }
    */
    return map_node;
}

