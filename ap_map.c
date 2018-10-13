#include "ap_map.h"
#include "ap_math.h"
#include "ap_macro.h"
#include "ap_snes.h"
#include "pq.h"

static struct ap_node map_nodes[0x100 * 0x100];
static size_t last_map_node_idx = -1;

static struct xy ap_targets[2048];
static size_t ap_target_count = 0;

static struct ap_node *
ap_node_append(struct ap_node * parent)
{
    struct ap_node * child = calloc(1, sizeof *child);
    assert(child != NULL);

    child->node_parent = parent;
    child->node_prev = parent->node_prev;
    child->node_prev->node_next = child;
    child->node_next = parent;
    child->node_next->node_prev = child;
    return child;
}

struct xy
ap_link_xy()
{
    return (struct xy) {
        .x = *ap_ram.link_x,
        .y = (uint16_t) (*ap_ram.link_y + 0),
    };
}

void
ap_map_bounds(struct xy * topleft, struct xy * bottomright)
{
    if (*ap_ram.in_building) {
        struct xy link = ap_link_xy();
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
    xy.x = tl.x | ((map16 & 0x03F) << 4);
    xy.y = tl.y | ((map16 & 0xFC0) >> 3);
    return xy;
}

void
ap_print_map()
{
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
ap_follow_targets(uint16_t * joypad)
{
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

static uint32_t
ap_path_heuristic(struct xy src, struct xy dst)
{
    //struct xy vec = XY(ABSDIFF(src.x, dst.x), ABSDIFF(src.y, dst.y));
    return XYL1DIST(src, dst);
}

static int
ap_pathfind_local(struct xy destination, bool commit)
{
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
            uint8_t tile = ap_tile_attrs[ap_map_attr(mapxy)];
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
ap_update_map_node()
{
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
        if (ap_tile_attrs[ap_map_attr(xy)] & walk_mask) {
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
        if (ap_tile_attrs[ap_map_attr(xy)] & walk_mask) {
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
        if (ap_tile_attrs[ap_map_attr(xy)] & walk_mask) {
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
        if (ap_tile_attrs[ap_map_attr(xy)] & walk_mask) {
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
    if (!*ap_ram.in_building) {
        for (size_t i = 0; i < 0x81; i++) {
            if (ap_ram.over_ent_areas[i] != *ap_ram.map_area)
                continue;
            new_node = ap_node_append(map_node);
            node_count++;
            new_node->tl = ap_map16_to_xy(tl, ap_ram.over_ent_map16s[i]);
            new_node->br = XYOP1(new_node->tl, + 15);
            snprintf(new_node->name, sizeof new_node->name, "door 0x%02x", ap_ram.over_ent_ids[i]);
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
            if ((ap_tile_attrs[attr] & TILE_ATTR_ITEM) || attr == 0x20) {
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

