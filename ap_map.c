#include "ap_map.h"
#include "ap_math.h"
#include "ap_macro.h"
#include "ap_snes.h"
#include "ap_plan.h"
#include "pq.h"

static struct ap_screen * map_screens[0x100 * 0x100];
static size_t last_map_index = -1;

static struct xy ap_targets[2048];
static size_t ap_target_count = 0;
static int ap_target_timeout = 0;

const char * const ap_node_type_names[] = {
#define X(type) [CONCAT(NODE_, type)] = #type,
NODE_TYPE_LIST
#undef X
};

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
ap_map_attr(struct xy xy)
{
    struct ap_screen * screen = map_screens[XYMAPSCREEN(xy)];
    if (screen == NULL) {
        LOG("null screen on xy:" PRIXYV, PRIXYVF(xy));
        return 0;
    }

    // Read from cache
    assert(XYIN(xy, screen->tl, screen->br));
    uint16_t y = (xy.y - screen->tl.y) / 8;
    uint16_t x = (xy.x - screen->tl.x) / 8;
    return screen->attr_cache[y][x];
}

static uint16_t
ap_map_attr_from_ram(struct xy point)
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

static void
ap_screen_refresh_cache(struct ap_screen * screen)
{
    for (uint16_t y = 0; y < 0x80; y++) {
        struct xy xy;
        xy.y = screen->tl.y + 8 * y;
        if (xy.y > screen->br.y) break;
        for (uint16_t x = 0; x < 0x80; x++) {
            xy.x = screen->tl.x + 8 * x;
            if (xy.x > screen->br.x) break;
            screen->attr_cache[y][x] = ap_map_attr_from_ram(xy);
        }
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

void
ap_print_screen(struct ap_screen * screen)
{
    FILE * mapf = fopen("map", "w");

    struct xy link = ap_link_xy();
    struct xy link_tl = link;
    struct xy link_br = XYOP1(link, + 15);
    if (screen == NULL) {
        screen = map_screens[XYMAPSCREEN(link_tl)];
    }

    for (struct xy xy = screen->tl; xy.y < screen->br.y; xy.y += 8) {
        for (xy.x = screen->tl.x; xy.x < screen->br.x; xy.x += 8) {
            uint8_t tile_attr = ap_map_attr(xy);
            if (XYIN(xy, link_tl, link_br)) {
                fprintf(mapf, TERM_GREEN(TERM_BOLD("%02x ")), tile_attr);
                goto next_point;
            } 
            for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
                if (XYIN(xy, node->tl, node->br)) {
                    fprintf(mapf, TERM_RED(TERM_BOLD("%02x ")), tile_attr);
                    goto next_point;
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
    fprintf(mapf, "Screen '%s': " PRIBBV "\n", screen->name, PRIBBVF(*screen));
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
            //LOG("L:" PRIXY "; done", PRIXYF(link));
            return RC_DONE;
        }

        target = ap_targets[ap_target_count-1];
        if (XYL1DIST(target, link) > 0)
            break;

        ap_target_count--;
    }

    INFO("L:" PRIXY "; %zu:" PRIXY "; %d", PRIXYF(link), ap_target_count, PRIXYF(ap_targets[ap_target_count-1]), ap_target_timeout);
    //LOG("L:" PRIXY "; %zu:" PRIXY "; %d", PRIXYF(link), ap_target_count, PRIXYF(ap_targets[ap_target_count-1]), ap_target_timeout);

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
ap_pathfind_local(struct ap_screen * screen, struct xy start_xy, struct xy destination_tl, struct xy destination_br, bool commit)
{
    if (commit)
        ap_set_targets(0);

    if (!XYIN(start_xy, screen->tl, screen->br)) {
        LOG("error: Start " PRIXYV "out of screen", PRIXYVF(start_xy));
        return -1;
    }
    if (!XYIN(destination_tl, screen->tl, screen->br)) {
        LOG("error: TL destination " PRIXYV "out of screen", PRIXYVF(destination_tl));
        return -1;
    }
    if (!XYIN(destination_br, screen->tl, screen->br)) {
        LOG("error: BR destination " PRIXYV "out of screen", PRIXYVF(destination_br));
        return -1;
    }

    struct xy size = XYOP2(screen->br, -, screen->tl);
    struct xy grid = XYOP1(size, / 8);
    struct xy tl_offset = XYOP1(screen->tl, / 8);

    struct xy src = XYOP2(XYOP1(start_xy, / 8), -, tl_offset);
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
            struct xy mapxy = XYOP2(XYOP1(XY(x, y), * 8), +, screen->tl);
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
    ap_print_screen(screen);
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
    return ap_pathfind_local(node->screen, ap_link_xy(), tl, br, true);
}

//map_screens[XYMAPSCREEN(tl)];

static void
ap_screen_commit_node(struct ap_screen * screen, struct ap_node ** new_node_p)
{
    struct ap_node * new_node = *new_node_p;
    NONNULL(new_node);

    if (new_node->tl.x == 0 && new_node->tl.y == 0)
        return;
    assert(new_node->type != NODE_NONE);

    // Check if the node already exists
    for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
        if (node->type != new_node->type)
            continue;
        if (node->adjacent_direction != new_node->adjacent_direction)
            continue;
        if (!XYIN(new_node->tl, node->tl, node->br) && !XYIN(new_node->br, node->tl, node->br))
            continue;
        assert(node->adjacent_screen == new_node->adjacent_screen);
        assert(node->tile_attr == new_node->tile_attr);

        LOG("duplicate node %s == %s", node->name, new_node->name);

        // Merge in new information???
        if (!XYEQ(node->tl, new_node->tl) || !XYEQ(node->br, new_node->br))
            LOG("node tl/br changed: before: " PRIBBV " after: " PRIBBV, PRIBBVF(*node), PRIBBVF(*new_node));

        node->tl = new_node->tl;
        node->br = new_node->br;

        memset(new_node, 0, sizeof *new_node);
        return;
    }

    // Attach to screen
    new_node->screen = screen;
    LL_PUSH(screen->node_list, new_node);
    LOG("attached node %s", new_node->name);

    // Add goals
    switch (new_node->type) {
    case NODE_TRANSITION:
        if (*new_node->adjacent_screen == NULL)
            ap_goal_add(GOAL_EXPLORE, new_node);
        break;
    case NODE_ITEM:
        ap_goal_add(GOAL_ITEM, new_node);
        break;
    case NODE_NONE:
    default:
        ;
    }

    *new_node_p = NONNULL(calloc(1, sizeof **new_node_p));
}

struct ap_screen *
ap_update_map_screen()
{
    struct xy tl, br;
    ap_map_bounds(&tl, &br);
    size_t index = XYMAPSCREEN(tl);
    if (index == last_map_index) {
        assert(map_screens[index] != NULL);
        return map_screens[index];
    }
    last_map_index = index;

    struct ap_screen * screen = map_screens[index];
    if (screen == NULL) {
        screen = calloc(1, sizeof *screen);
        screen->tl = tl;
        screen->br = br;
        LL_INIT(screen->node_list);
        snprintf(screen->name, sizeof screen->name, PRIXYV " x " PRIXYV, PRIXYVF(tl), PRIXYVF(br));

        struct xy xy;
        for (xy.y = tl.y; xy.y < br.y; xy.y += 0x100) {
            for (xy.x = tl.x; xy.x < br.x; xy.x += 0x100) {
                map_screens[XYMAPSCREEN(xy)] = screen;
            }
        }
    }
    assert(screen != NULL);
    ap_screen_refresh_cache(screen);

    int new_node_count = 0;
    static struct ap_node * new_node = NULL;
    if (new_node == NULL)
        new_node = NONNULL(calloc(1, sizeof *new_node));

    uint16_t walk_mask = TILE_ATTR_WALK | TILE_ATTR_DOOR;
    const struct xy xy_init[5] = {{0},
        tl, XY(tl.x, br.y & ~7), tl, XY(br.x & ~7, tl.y) };
    const struct xy xy_step[5] = {{0},
        dir_dxy[DIR_R], dir_dxy[DIR_R], dir_dxy[DIR_D], dir_dxy[DIR_D] };

    for (uint8_t i = 1; i < 5; i++) {
        struct xy new_start = XY(0, 0);
        struct xy new_end = XY(0, 0);

        for (struct xy xy = xy_init[i]; XYIN(xy, tl, br); xy = XYOP2(xy, +, XYOP1(xy_step[i], * 8))) {
            bool can_walk = true;
            for (int8_t j = 0; j < 3; j++) {
                struct xy lxy = XYOP2(xy, -, XYOP1(dir_dxy[i], * 8 * j));
                if (!(ap_tile_attrs[ap_map_attr(lxy)] & walk_mask)) {
                    can_walk = false;
                    break;
                }
            }
            if (can_walk) {
                new_end = XYOP2(xy, -, XYOP1(dir_dxy[i], * 8 * 2));
                if (XYEQ(new_start, XY(0, 0))) {
                    new_start = XYOP2(xy, -, XYOP1(dir_dxy[i], * 8 * 1));
                }
            } else if (!XYEQ(new_start, XY(0, 0))) {
                new_node->tl = XYFN2(MIN, new_start, new_end);
                new_node->br = XYFN2(MAX, new_start, new_end);
                new_node->br = XYOP1(new_node->br, + 7);
                new_node->type = NODE_TRANSITION;
                new_node->adjacent_direction = i;
                new_node->adjacent_screen = &map_screens[XYMAPSCREEN(XYOP2(new_node->tl, +, XYOP1(dir_dxy[i], * 0x100)))];
                snprintf(new_node->name, sizeof new_node->name, "0x%02zX %s %d", index, dir_names[i], ++new_node_count);
                ap_screen_commit_node(screen, &new_node);
                new_start = new_end = XY(0, 0);
            }
        }
        if (!XYEQ(new_start, XY(0, 0))) {
            new_node->tl = XYFN2(MIN, new_start, new_end);
            new_node->br = XYFN2(MAX, new_start, new_end);
            new_node->br = XYOP1(new_node->br, + 7);
            new_node->type = NODE_TRANSITION;
            new_node->adjacent_direction = i;
            new_node->adjacent_screen = &map_screens[XYMAPSCREEN(XYOP2(new_node->tl, +, XYOP1(dir_dxy[i], * 0x100)))];
            snprintf(new_node->name, sizeof new_node->name, "0x%02zX %s %d", index, dir_names[i], ++new_node_count);
            ap_screen_commit_node(screen, &new_node);
        }
    }
    if (!*ap_ram.in_building && false) {
        LOG("xo: %04x, xm: %04x, yo: %04x, ym: %04x",
            *ap_ram.map_x_offset, *ap_ram.map_x_mask, *ap_ram.map_y_offset, *ap_ram.map_y_mask);
        for (size_t i = 0; i < 0x81; i++) {
            if (ap_ram.over_ent_areas[i] != *ap_ram.map_area)
                continue;

            uint16_t id = ap_ram.over_ent_ids[i];
            if (id > 0x85) {
                LOG("weird id: %u %zu", id, i);
                continue;
            }

            new_node->tl = ap_map16_to_xy(tl, ap_ram.over_ent_map16s[i]);
            new_node->br = XYOP1(new_node->tl, + 15);
            new_node->type = NODE_TRANSITION;
            new_node->tile_attr = 0x80; // not quite true
            struct xy entrance = XY(ap_ram.entrance_xs[id], ap_ram.entrance_ys[id]);
            new_node->adjacent_screen = &map_screens[XYMAPSCREEN(entrance)];
            new_node->adjacent_direction = DIR_U;
            snprintf(new_node->name, sizeof new_node->name, "door 0x%02x", ap_ram.over_ent_ids[i]);
            LOG("tl: " PRIXYV ", out: " PRIXYV ", map16: %04x", PRIXYVF(tl), PRIXYVF(new_node->tl), ap_ram.over_ent_map16s[i]);
            ap_screen_commit_node(screen, &new_node);
            new_node_count++;
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
    for (struct xy xy = tl; xy.y < br.y; xy.y += 0x10) {
        for (xy.x = tl.x; xy.x < br.x; xy.x += 0x10) {
            uint8_t attr = ap_map_attr(xy);
            if (ap_tile_attrs[attr] & TILE_ATTR_ITEM) {
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
                new_node->type = NODE_ITEM;
                new_node->tile_attr = attr;
                snprintf(new_node->name, sizeof new_node->name, "item 0x%02x", attr);
                ap_screen_commit_node(screen, &new_node);
                new_node_count++;
            }
        }
    }

    ap_print_screen(screen);
    printf("found %d nodes\n", new_node_count);
    int i = 0; 
    for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
        bool reachable = ap_pathfind_local(screen, ap_link_xy(), node->tl, node->br, false) > 0;
        node->_reachable = reachable;
        printf("   %d. [%c %c] (" PRIXY " x " PRIXY ") %s\n",
                i++, "!."[reachable], "?."[node->adjacent_node != NULL], PRIXYF(node->tl), PRIXYF(node->br), node->name);
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
    return screen;
}

