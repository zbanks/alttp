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
static int ap_target_subtimeout = 0;
static struct xy ap_target_dst_tl;
static struct xy ap_target_dst_br;
static struct ap_screen * ap_target_screen;

const char * const ap_node_type_names[] = {
#define X(type) [CONCAT(NODE_, type)] = #type,
NODE_TYPE_LIST
#undef X
};

static int
ap_pathfind_local(struct ap_screen * screen, struct xy start_xy, struct xy destination_tl, struct xy destination_br, bool commit);

struct xy
ap_link_xy()
{
    // Returns the effective tl coordinate that link interacts with
    // Which is 8 pixels below his head
    struct xy link = XY(*ap_ram.link_x, *ap_ram.link_y + 8);
    if (*ap_ram.in_building) {
        link.x = ((link.x & ~0x1FF) << 2) | (link.x & 0x1FF);
        link.x += 0x4000;
        if (!*ap_ram.link_lower_level) {
            link.x += 0x200;
        }
    }
    return link;
}

struct xy
ap_sprite_xy(uint8_t i) {
    assert(i < 16);
    struct xy sprite = XY(ap_ram.sprite_x_lo[i], ap_ram.sprite_y_lo[i]);
    sprite.x += ap_ram.sprite_x_hi[i] << 8u;
    sprite.y += ap_ram.sprite_y_hi[i] << 8u;

    if (*ap_ram.in_building) {
        sprite.x = ((sprite.x & ~0x1FF) << 2) | (sprite.x & 0x1FF);
        sprite.x += 0x4000;
        if (!*ap_ram.sprite_lower_level) {
            sprite.x += 0x200;
        }
    }
    return sprite;
}

void
ap_map_bounds(struct xy * topleft, struct xy * bottomright)
{
    if (*ap_ram.in_building) {
        struct xy link = ap_link_xy();
        link.y = CAST16(link.y - 8);
        topleft->x = (uint16_t) (link.x & ~1023);
        topleft->y = (uint16_t) (link.y & ~511);
        bottomright->x = (uint16_t) (topleft->x | 1023);
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
        //LOG("null screen on xy:" PRIXYV, PRIXYVF(xy));
        return 0;
    }

    // Read from cache
    assert(XYIN(xy, screen->tl, screen->br));
    uint16_t y = (xy.y - screen->tl.y) / 8;
    uint16_t x = (xy.x - screen->tl.x) / 8;
    return screen->attr_cache[y][x];
}

static uint16_t
ap_link_tile_attr(struct xy link)
{
    uint16_t tile = 0;
    tile |= ap_tile_attrs[ap_map_attr(XYOP2(link, +, XY(0, 0)))];
    tile |= ap_tile_attrs[ap_map_attr(XYOP2(link, +, XY(0, 8)))];
    tile |= ap_tile_attrs[ap_map_attr(XYOP2(link, +, XY(8, 0)))];
    tile |= ap_tile_attrs[ap_map_attr(XYOP2(link, +, XY(8, 8)))];
    return tile;
}

static uint16_t
ap_map_attr_from_ram(struct xy point)
{
    if (*ap_ram.in_building) {
        assert(point.x >= 0x4000);
        if (point.x & 0x200) {
            return ap_ram.dngn_bg2_tattr[XYMAP8(point)];
        } else  {
            // lower level
            return ap_ram.dngn_bg1_tattr[XYMAP8(point)];
        }
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
    xy.x = tl.x + ((map16 & 0x07E) << 3) + 8;
    xy.y = tl.y + ((map16 & 0x1F80) >> 3) + 16;
    if (XYEQ(tl, XY(0x0c00, 0x0600))) {
        // XXX: Not sure why this correction is nessassary?
        //xy = XYOP2(xy, -, XY(8, 16));
        xy.x = tl.x + ((map16 & 0x07F) << 3);
        xy.y = tl.y + ((map16 & 0xFC0) >> 3);
    }
    return xy;
}

void
ap_print_map_screen(struct ap_screen * screen)
{
    FILE * mapf = fopen("map", "w");
    FILE * mapimg = fopen("map.pbm.tmp", "w");

    struct xy link = ap_link_xy();
    struct xy link_tl = link;
    struct xy link_br = XYOP1(link, + 15);
    if (screen == NULL) {
        screen = map_screens[XYMAPSCREEN(link_tl)];
    }

    struct xy screen_size = XYOP1(XYOP2(screen->br, -, screen->tl), / 8 + 1);
    fprintf(mapimg, "P6 %u %u %u\n", screen_size.x, screen_size.y, 255);

    uint16_t semi_tile_attrs = TILE_ATTR_LFT0 | TILE_ATTR_DOOR;

    for (struct xy xy = screen->tl; xy.y < screen->br.y; xy.y += 8) {
        for (xy.x = screen->tl.x; xy.x < screen->br.x; xy.x += 8) {
            uint8_t tile_attr = ap_map_attr(xy);
            bool tile_walk = ap_tile_attrs[tile_attr] & TILE_ATTR_WALK;
            bool tile_semi = ap_tile_attrs[tile_attr] & semi_tile_attrs;
            uint8_t px_shade = tile_walk ? 255 : (tile_semi ? 127 : 0);
            if (XYIN(xy, link_tl, link_br)) {
                fprintf(mapf, TERM_GREEN(TERM_BOLD("%02x ")), tile_attr);
                fprintf(mapimg, "%c%c%c", 0, 128, 0);
                goto next_point;
            } 
            for (uint8_t i = 0; i < 16; i++) {
                if (ap_ram.sprite_type[i] == 0) {
                    continue;
                }
                struct xy sprite_tl = ap_sprite_xy(i);
                struct xy sprite_br = XYOP1(sprite_tl, + 15); // TODO?
                if (XYIN(xy, sprite_tl, sprite_br)) {
                    if ((xy.x ^ xy.y) & 0x8) {
                        fprintf(mapf, TERM_MAGENTA("%02x "), tile_attr);
                    } else {
                        fprintf(mapf, TERM_MAGENTA(TERM_BOLD("%02x ")), ap_ram.sprite_type[i]);
                    }
                    fprintf(mapimg, "%c%c%c", 255, px_shade / 4, 255);
                    goto next_point;
                }
            }
            for (struct ap_goal * goal = ap_goal_list->next; goal != ap_goal_list; goal = goal->next) {
                if (XYIN(xy, goal->node->tl, goal->node->br)) {
                    fprintf(mapf, TERM_YELLOW("%02x "), tile_attr);
                    fprintf(mapimg, "%c%c%c", 255, 255 - goal->attempts * 64, 0);
                    goto next_point;
                }
            }
            for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
                if (XYIN(xy, node->tl, node->br)) {
                    fprintf(mapf, TERM_RED(TERM_BOLD("%02x ")), tile_attr);
                    if (node->adjacent_node == NULL) {
                        fprintf(mapimg, "%c%c%c", 255, 0, px_shade / 4);
                    } else {
                        fprintf(mapimg, "%c%c%c", 128, 255, 0);
                    }
                    goto next_point;
                }
            }
            for (size_t i = 0; i < ap_target_count; i++) {
                if (XYIN(xy, ap_targets[i], XYOP1(ap_targets[i], +8))) {
                    fprintf(mapf, TERM_BLUE(TERM_BOLD("%02x ")), tile_attr);
                    fprintf(mapimg, "%c%c%c", 0, px_shade / 4, 255);
                    goto next_point;
                }
            }
            fprintf(mapimg, "%c%c%c", px_shade, px_shade, px_shade);
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
    fclose(mapf);
    fclose(mapimg);
    rename("map.pbm.tmp", "map.pbm");
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
    //fprintf(stderr, "(dest) ");
    for (size_t i = 0; i < count; i++) {
        if (XYEQ(ap_targets[i], XY(0,0))) {
            LOG("target 0");
            assert_bp(false);
        }
        ap_target_timeout += XYL1DIST(ap_targets[i], ap_targets[i+1]) * 2;
        //fprintf(stderr, PRIXYV ", ", PRIXYVF(ap_targets[i]));
    }
    //fprintf(stderr, PRIXYV " (link)\n", PRIXYVF(ap_targets[count]));

    for (size_t i = count; i < sizeof(ap_targets) / sizeof(*ap_targets); i++) {
        ap_targets[count] = XY(0, 0);
    }

    ap_target_subtimeout = 32;
    ap_target_timeout += 64;
    fflush(NULL);
    LOG("Following targets with timeout: %d", ap_target_timeout);
    ap_print_map_screen(NULL);
    return ap_target_timeout;
}

int
ap_follow_targets(uint16_t * joypad)
{
    struct xy link = ap_link_xy();
    if (ap_target_timeout <= 0) {
        //INFO("L:" PRIXY "; " PRIXY " timeout", PRIXYF(link), PRIXYF(ap_targets[0]));
        LOG("L:" PRIXY "; " PRIXY " timeout", PRIXYF(link), PRIXYF(ap_targets[0]));
        ap_target_count = 0;
        return RC_FAIL;
    }

    if (ap_target_subtimeout-- <= 0) {
        int prev_timeout = ap_target_timeout;
        int rc = ap_pathfind_local(NULL, link, ap_target_dst_tl, ap_target_dst_br, true);
        assert_bp(rc >= 0);
        ap_target_timeout = MIN(prev_timeout, ap_target_timeout);
    }

    struct xy target;
    while (true) {
        if (ap_target_count < 1) {
            //INFO("L:" PRIXY "; done", PRIXYF(link));
            //LOG("L:" PRIXY "; done", PRIXYF(link));
            return RC_DONE;
        }

        target = ap_targets[ap_target_count-1];
        if (XYL1DIST(link, target) == 0) {
            ap_target_count--;
            continue;
        }
        if (ap_target_count > 1) {
            struct xy next_target = ap_targets[ap_target_count-2];
            if (XYIN(link, XYFN2(MIN, target, next_target), XYFN2(MAX, target, next_target))) {
                ap_target_count--;
                continue;
            }
        }

        break;
    }

    if (XYLINKIN(link, ap_target_dst_tl, ap_target_dst_br)) {
        ap_target_count = 0;
        return RC_DONE;
    }

    if (!XYIN(link, ap_target_screen->tl, ap_target_screen->br)) {
        LOG("Link left the screen!");
        assert_bp(false);
        return RC_FAIL;
    }

    INFO("L:" PRIXY "; %zu:" PRIXY "; %d %#x", PRIXYF(link), ap_target_count, PRIXYF(ap_targets[ap_target_count-1]), ap_target_timeout, *ap_ram.push_dir_bitmask);
    //LOG("L:" PRIXY "; %zu:" PRIXY "; %d", PRIXYF(link), ap_target_count, PRIXYF(ap_targets[ap_target_count-1]), ap_target_timeout);

    if (link.x > target.x)
        JOYPAD_SET(LEFT);
    else if (link.x < target.x)
        JOYPAD_SET(RIGHT);
    if (link.y > target.y)
        JOYPAD_SET(UP);
    else if (link.y < target.y)
        JOYPAD_SET(DOWN);
    /*
    uint8_t dir = 0;
    if (link.x > target.x) {
        if (link.y > target.y)
            dir = DIR_LU;
        else if (link.y < target.y)
            dir = DIR_LD;
        else
            dir = DIR_L;
    } else if (link.x < target.x) {
        if (link.y > target.y)
            dir = DIR_RU;
        else if (link.y < target.y)
            dir = DIR_RD;
        else
            dir = DIR_R;
    } else {
        if (link.y > target.y)
            dir = DIR_U;
        else if (link.y < target.y)
            dir = DIR_D;
        else
            dir = 0;
    }
    if (dir != 0) {
        *joypad |= dir_joypad[dir];
    }
    */
    
    /*
    uint16_t tile_mask = TILE_ATTR_WALK | TILE_ATTR_DOOR | TILE_ATTR_LDGE | TILE_ATTR_LFT0;
    if (link.x > target.x && (ap_link_tile_attr(XYOP2(link, -, XY(8, 0))) & tile_mask))
        JOYPAD_SET(LEFT);
    else if (link.x < target.x && (ap_link_tile_attr(XYOP2(link, +, XY(8, 0))) & tile_mask))
        JOYPAD_SET(RIGHT);
    if (link.y > target.y && (ap_link_tile_attr(XYOP2(link, -, XY(0, 8))) & tile_mask))
        JOYPAD_SET(UP);
    else if (link.y < target.y && (ap_link_tile_attr(XYOP2(link, +, XY(0, 8))) & tile_mask))
        JOYPAD_SET(DOWN);
    */

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
    if (src.x >= 0x4000 && dst_tl.x >= 0x4000 && dst_br.x >= 0x4000) {
        // collapse BG1 & BG2
        if ((src.x & ~0x200) != (dst_tl.x & ~0x200)) {
            distance += 1;
        }
        distance += 1;
        src.x &= ~0x200;
        dst_tl.x &= ~0x200;
        dst_br.x &= ~0x200;
    }
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

    if (screen == NULL) {
        screen = map_screens[XYMAPSCREEN(start_xy)];
    } else {
        assert_bp(screen == map_screens[XYMAPSCREEN(start_xy)]);
    }

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

    if (commit) {
        //LOG("Starting pathfind in screen " PRIBBV ", start " PRIXYV, PRIBBVF(*screen), PRIXYVF(start_xy));
    }

    start_xy = XYFN2(MAX, start_xy, XYOP1(screen->tl, + 16));
    start_xy = XYFN2(MIN, start_xy, XYOP1(screen->br, - 16));

    struct xy size = XYOP2(screen->br, -, screen->tl);
    struct xy grid = XYOP1(size, / 8 + 1);
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
        uint16_t tile_attrs;
        uint8_t raw_tile;
        uint8_t ledge;
        uint32_t gscore;
        uint32_t fscore;
        uint32_t cost;
    } buf[0x82 * 0x82]; 
    if (grid.x * grid.y > 0x80 * 0x80) {
        printf("error: grid too large: " PRIXY "\n", PRIXYF(grid));
        return -1;
    }
    for (size_t i = 0; i < 0x82 * 0x82; i++) {
        buf[i].ledge = 0xFF;
    }
    // Make a state[y][x]
    struct point_state (*state)[grid.x + 2] = (void *) &buf[0x83];

    for (uint16_t y = 0; y < grid.y; y++) {
        for (uint16_t x = 0; x < grid.x; x++) {
            assert_bp(&state[y][x] == &buf[0x83 + (grid.x + 2) * y + x]);
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
            uint8_t raw_tile = ap_map_attr(mapxy);
            uint16_t tile = ap_tile_attrs[raw_tile];
            state[y][x].tile_attrs = tile;
            state[y][x].raw_tile = raw_tile;
            uint8_t ledge_mask = 0;
            if (tile & (TILE_ATTR_WALK | TILE_ATTR_DOOR)) {
                cost = 0;
            } else if (tile & TILE_ATTR_LDGE) {
                //cost = (1 << 20); // calculated at search time
                cost = 0;
                assert_bp((raw_tile &~0x7) == 0x28);
                ledge_mask = 1ul << (raw_tile & 0x7);
            } else if (tile & TILE_ATTR_LFT0) {
                cost = 40;
            } else if (XYIN(mapxy, dst_tl, dst_br)) {
                cost = 0;
            } else if (raw_tile == 0x1C) {
                cost = 1000;
            } else {
                cost = (1 << 20);
            }

            state[y-0][x-0].cost += cost;
            state[y-0][x-1].cost += cost;
            state[y-1][x-0].cost += cost;
            state[y-1][x-1].cost += cost;

            state[y-0][x-0].ledge |= ledge_mask;
            state[y-0][x-1].ledge |= ledge_mask;
            state[y-1][x-0].ledge |= ledge_mask;
            state[y-1][x-1].ledge |= ledge_mask;

            if (x < 1 || x >= grid.x - 1 ||
                y < 1 || y >= grid.y - 1) {
                state[y][x].cost = (1 << 20);
            } else if (x < 3 || x >= grid.x - 3 ||
                y < 3 || y >= grid.y - 3) {
                state[y][x].cost += 10000;
            }
        }
    }
    for (volatile uint8_t i = 0; i < 16; i++) {
        if (ap_ram.sprite_type[i] == 0) {
            continue;
        }
        struct xy sprite = ap_sprite_xy(i);
        sprite = XYOP2(XYOP1(sprite, / 8), -, tl_offset);
        uint16_t sprite_attrs = ap_sprite_attrs[ap_ram.sprite_type[i]];
        if (sprite_attrs & SPRITE_ATTR_ENMY && 
            ap_ram.sprite_state[i] != 0x00) { // dead
            for (int dx = -4; dx <= 4; dx++) {
                for (int dy = -4; dy <= 4; dy++) {
                    struct xy ds = XYOP2(sprite, +, XY(dx, dy));
                    if (!XYUNDER(ds, grid)) {
                        continue;
                    }
                    state[ds.y][ds.x].cost += (ABS(dx) + ABS(dy)) * 50;
                }
            }
        }
        if (sprite_attrs & SPRITE_ATTR_BLOK &&
            ap_ram.sprite_state[i] != 0x0A && // carried
            ap_ram.sprite_state[i] != 0x06) { // thrown
            assert_bp(ap_ram.sprite_state[i] == 0x08 || 
                      ap_ram.sprite_state[i] == 0x09 || 
                      ap_ram.sprite_state[i] == 0x00);
            for (int dx = 0; dx <= 1; dx++) {
                for (int dy = 0; dy <= 1; dy++) {
                    struct xy ds = XYOP2(sprite, +, XY(dx, dy));
                    if (!XYUNDER(ds, grid)) {
                        continue;
                    }
                    state[ds.y][ds.x].cost += (1 << 20);
                }
            }
        }
    }
    // Hack to round when there is an obstacle nearby
    // Doesn't handle diagonals perfectly, but should be fine?
    if ((start_xy.x & 0x7) != 0 &&
        state[src.y][src.x].cost > state[src.y][src.x+1].cost) {
        src.x += 1;
        if (commit) LOG("Nudged X start");
    }
    if ((start_xy.y & 0x7) != 0 &&
        state[src.y][src.x].cost > state[src.y+1][src.x].cost) {
        src.y += 1;
        if (commit) LOG("Nudged Y start");
    }

    static struct pq * pq = NULL;
    if (pq == NULL) {
        pq = pq_create(sizeof(struct xy));
        if (pq == NULL) exit(1);
    }
    pq_clear(pq);
    pq_push(pq, 0, &src);

    struct point_state start_ps = { .xy = start_xy };
    state[src.y][src.x].from = &start_ps;
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
        assert_bp(state[node.y][node.x].ledge != 0xFF);
        for (uint8_t i = 1; i < 9; i++) {
            struct xy neighbor = XYOP2(node, +, dir_dxy[i]);
            if (neighbor.x >= grid.x || neighbor.y >= grid.y)
                continue;
            assert_bp(state[neighbor.y][neighbor.x].ledge != 0xFF);
            if (state[neighbor.y][neighbor.x].raw_tile == 0x1C)
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
            // Handle stairs
            if (state[node.y][node.x].raw_tile == 0x3E ||
                state[node.y][node.x].raw_tile == 0x1E) {
                neighbor.x ^= 0x200 / 8;
                //neighbor = XYOP2(neighbor, +, dir_dxy[i]);
                //neighbor = XYOP2(neighbor, +, dir_dxy[i]);
                assert_bp(XYUNDER(neighbor, grid));
            }
            // Jump down ledges
            uint8_t ledge_mask = 1ul << (i - 1);
            assert(ledge_mask != 0);
            if (false && (state[node.y][node.x].ledge & ledge_mask)) {
                bool fail_ledge = false;
                int iters = 0;
                while (true) {
                    if (!XYUNDER(neighbor, grid)) {
                        fail_ledge = true;
                        break;
                    }
                    if (state[neighbor.y][neighbor.x].raw_tile == 0x1C) {
                        assert_bp(neighbor.x & (0x200 / 8));
                        neighbor.x -= 0x200 / 8;
                        neighbor = XYOP2(neighbor, +, dir_dxy[i]);
                        assert_bp(XYUNDER(neighbor, grid));
                    } else if (state[neighbor.y][neighbor.x].cost < (1 << 20) &&
                        state[neighbor.y][neighbor.x].ledge == 0) {
                        break;
                    }
                    if (state[neighbor.y][neighbor.x].tile_attrs & TILE_ATTR_SWIM) {
                        fail_ledge = true;
                        break;
                    }
                    //assert_bp(state[neighbor.y][neighbor.x].ledge != 0xFF);
                    iters++;
                    struct xy next_neighbor = XYOP2(neighbor, +, dir_dxy[i]);
                    if (!XYUNDER(next_neighbor, grid))
                        break;
                    neighbor = next_neighbor;
                }
                if (fail_ledge)
                    continue;
                if (commit) LOG("Ledge %u in %d iters from " PRIXYV " to " PRIXYV, i, iters, PRIXYVF(node), PRIXYVF(neighbor));
                assert_bp(XYUNDER(neighbor, grid));
            } else if (state[node.y][node.x].ledge != 0) {
                continue;
            }
            assert_bp(XYUNDER(neighbor, grid));

            uint32_t gscore = state[node.y][node.x].gscore;
            gscore += dir_cost[i];
            if (!(state[node.y][node.x].ledge & ledge_mask)) {
                uint32_t step_cost = state[neighbor.y][neighbor.x].cost;
                if (i >= 5) {
                    step_cost = MAX(step_cost, state[neighbor.y][node.x].cost);
                    step_cost = MAX(step_cost, state[node.y][neighbor.x].cost);
                }
                gscore += step_cost;
            }
            if (gscore >= (1 << 20))
                continue;
            if (gscore >= state[neighbor.y][neighbor.x].gscore)
                continue;

            uint32_t heuristic = ap_path_heuristic(neighbor, dst_tl, dst_br);
            if (heuristic == 0) {
                assert_bp(XYIN(neighbor, dst_tl, dst_br));
                final_xy = neighbor;
                state[neighbor.y][neighbor.x].from = &state[node.y][node.x];
                state[neighbor.y][neighbor.x].gscore = gscore;
                goto search_done;
            }

            min_heuristic = MIN(heuristic, min_heuristic);
            uint32_t fscore = gscore + heuristic;
            state[neighbor.y][neighbor.x].from = &state[node.y][node.x];
            state[neighbor.y][neighbor.x].gscore = gscore;
            state[neighbor.y][neighbor.x].fscore = fscore;
            
            // Issue: neighbor.y == grid.y
            assert_bp(XYUNDER(neighbor, grid));
            assert_bp(state[neighbor.y][neighbor.x].ledge != 0xFF);
            pq_push(pq, fscore, &neighbor);
        }
    }
    if (commit) LOG("A* timed out, min heuristic: %lu", min_heuristic);
    return -1;
search_failed:
    if (commit) LOG("A* failed, min heuristic: %lu", min_heuristic);
    return -1;
search_done:;
    //if (commit) LOG("A* done in %zu steps, pq size = %zu", r, pq_size(pq));

    struct point_state * next = &state[final_xy.y][final_xy.x];
    int final_gscore = next->gscore;
    if (!commit) {
        return final_gscore;
    }

    size_t count = 0;
    ap_targets[count++] = XY(
            MIN(next->xy.x, destination_br.x - 15),
            MIN(next->xy.y, destination_br.y - 15));

    struct xy last_dir = XY(1, 2);
    struct xy last_xy = next->xy;
    while (next != NULL && next->from != NULL) {
        struct xy dir = XYOP2(next->xy, -, last_xy);
        assert_bp(next->from == NULL || !XYEQ(next->from->xy, XY(0,0)));
        assert_bp(next->raw_tile != 0x10);

        if (!next->ledge) {
            if (true || !XYEQ(dir, last_dir)) {
                //XXX This is normally bad, but for now assume recoil
                //assert_bp(!((next->xy.x <= screen->tl.x + 15) || (next->xy.y <= screen->tl.y + 15)));
                //assert_bp(!((next->xy.x >= screen->br.x - 15) || (next->xy.y >= screen->br.y - 15)));
                ap_targets[count++] = next->xy;
                //printf("%zu: "PRIXY "\n", count, PRIXYF(ap_targets[count-1]));
            }
        }
        last_dir = dir;
        last_xy = next->xy;
        next = next->from;
    }

    int timeout = ap_set_targets(count);
    ap_target_dst_tl = destination_tl;
    ap_target_dst_br = destination_br;
    ap_target_screen = screen;
    ap_print_map_screen(screen);
    return final_gscore;
}

static int
ap_pathfind_global(struct xy start_xy, struct ap_node * destination, bool commit)
{
    struct ap_screen * start_screen = map_screens[XYMAPSCREEN(start_xy)];
    if (start_screen == NULL) {
        LOG("start screen == NULL");
        return -1;
    }
    if (start_screen == destination->screen) {
        return ap_pathfind_local(start_screen, start_xy, destination->tl, destination->br, commit);
    }

    //if (commit) LOG("Starting global search from " PRIXYV " to " PRIBBV, PRIXYVF(start_xy), PRIBBVF(*destination));

    static struct pq * pq = NULL;
    if (pq == NULL) {
        pq = pq_create(sizeof(struct ap_node *));
        if (pq == NULL) exit(1);
    }
    pq_clear(pq);

    static uint64_t iter = 0;
    iter += 2;
    static struct ap_node _start_node;
    struct ap_node * start_node = &_start_node;
    *start_node = (struct ap_node) {
        .screen = start_screen,
        .pgsearch = {
            .iter = iter,
            .xy = start_xy,
            .from = NULL,
            .distance = 0,
        }
    };
    pq_push(pq, 0, &start_node);

    size_t r = 0;
    while (true) {
        r++;
        struct ap_node * node;
        uint64_t distance;
        if (pq_pop(pq, &distance, &node) < 0) 
            goto search_failed;
        if (node->pgsearch.iter == iter + 1)
            continue;
        assert(node->pgsearch.iter == iter);
        node->pgsearch.iter++;
        if (node == destination)
            goto search_done;
        assert(node->type == NODE_TRANSITION || node == start_node);

        // Try going to an adjacent screen
        if (node->adjacent_node != NULL) {
            uint64_t distance = node->pgsearch.distance + 10000;
            if ((node->adjacent_node->pgsearch.iter == iter &&
                 node->adjacent_node->pgsearch.distance < distance) ||
                 node->adjacent_node->pgsearch.iter < iter) {
                node->adjacent_node->pgsearch = (struct ap_node_pgsearch) {
                    .iter = iter,
                    .xy = XYMID(node->adjacent_node->tl, node->adjacent_node->br),
                    .from = node,
                    .distance = distance,
                };
                pq_push(pq, distance, &node->adjacent_node);
            }
        }
        
        // Try going to nodes on the same screen
        for (struct ap_node * adj_node = node->screen->node_list->next; adj_node != node->screen->node_list; adj_node = adj_node->next) {
            if (adj_node == node)
                continue;
            if (!(adj_node == destination || adj_node->type == NODE_TRANSITION))
                continue;
            if (adj_node->pgsearch.iter == iter + 1)
                continue;
            int delta_distance = ap_pathfind_local(node->screen, node->pgsearch.xy, adj_node->tl, adj_node->br, false);
            if (delta_distance < 0)
                continue;

            uint64_t distance = node->pgsearch.distance + delta_distance;
            if ((adj_node->pgsearch.iter == iter &&
                 adj_node->pgsearch.distance > distance) ||
                 adj_node->pgsearch.iter < iter) {
                 adj_node->pgsearch = (struct ap_node_pgsearch) {
                    .iter = iter,
                    .xy = XYMID(adj_node->tl, adj_node->br),
                    .from = node,
                    .distance = distance,
                };
                pq_push(pq, distance, &adj_node);
            }
        }
    }
search_failed:
    if (commit) LOG("Search failed after %zu steps", r);
    return -1;
search_done:
    LOG("Search done in %zu steps, pq size = %zu", r, pq_size(pq));
    if (commit) LOG("Search done in %zu steps, pq size = %zu", r, pq_size(pq));
    else return destination->pgsearch.distance;

    struct ap_node * next = destination;
    size_t count = 0;
    while (next != NULL) {
        assert(next->pgsearch.iter == iter + 1);
        LOG(" %zu: %s " PRIXYV " %ld", count, next->name, PRIXYVF(next->pgsearch.xy), next->pgsearch.distance);
        next = next->pgsearch.from;
        count++;
    }

    return destination->pgsearch.distance;
}

void
ap_print_map_full()
{
    ap_print_state();
    FILE * mapf = fopen("full_map.pgm.tmp", "w");
    fprintf(mapf, "P6 %u %u %u\n", 0xA00, 0x600, 255);
    //fprintf(mapf, "P6 %u %u %u\n", 0x800, 0x800, 255);

    struct xy link = ap_link_xy();
    struct xy link_tl = link;
    struct xy link_br = XYOP1(link, + 15);

    // Overworld: (0, 0)
    // Dark World: (0, 1)?
    // Underworld: (4, 0) x (7, 2)?
    // Zora's Domain: ?
    // Pedistal: ?
    //for (struct xy xy = XY(0, 0); xy.y < 0x8000; xy.y += 8) {
    for (struct xy xy = XY(0, 0); xy.y < 0x3000; xy.y += 8) {
        //for (xy.x = 0; xy.x < 0x8000; xy.x += 8) {
        for (xy.x = 0; xy.x < 0x8000; xy.x += 8) {
            if (xy.x >= 0x1000 && xy.x < 0x4000) continue;
            //if ((xy.y & 0x2000) || (xy.x & 0x2000)) continue;
            if ((xy.y % 0x1000) == 0 || (xy.x % 0x1000) == 0) {
                fputc(200, mapf);
                fputc(0, mapf);
                fputc(200, mapf);
                goto next_point;
            }
            uint8_t tile_attr = 0x80; // default: grey
            struct ap_screen * screen = map_screens[XYMAPSCREEN(xy)];
            if (screen != NULL) {
                tile_attr = ap_map_attr(xy);
            }
            if (XYIN(xy, link_tl, link_br)) {
                fputc(0, mapf);
                fputc(128, mapf);
                fputc(0, mapf);
                goto next_point;
            }
            for (size_t i = 0; i < ap_target_count; i++) {
                if (XYIN(xy, ap_targets[i], XYOP1(ap_targets[i], +8))) {
                    fputc(0, mapf);
                    fputc(255 - tile_attr, mapf);
                    fputc(255, mapf);
                    goto next_point;
                }
            }
            for (struct ap_goal * goal = ap_goal_list->next; goal != ap_goal_list; goal = goal->next) {
                if (XYIN(xy, goal->node->tl, goal->node->br)) {
                    fprintf(mapf, "%c%c%c", 255, 255 - goal->attempts * 64, 0);
                    goto next_point;
                }
            }
            if (screen != NULL) {
                for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
                    if (node->adjacent_node == NULL)
                        continue;
                    if (!XYIN(xy, node->tl, node->br))
                        continue;
                    fprintf(mapf, "%c%c%c", 128, 255, 0);
                    goto next_point;
                }
            }
            fputc(255 - tile_attr, mapf);
            fputc(255 - tile_attr, mapf);
            fputc(255 - tile_attr, mapf);
next_point:;
        }
    }
    fclose(mapf);
    rename("full_map.pgm.tmp", "full_map.pgm");
    LOG("Exported full map");
}

void
ap_print_state()
{
    FILE * mapf = fopen("map_attrs.pgm", "w");
    fprintf(mapf, "P5 %u %u %u\n", 0x1000, 0x1000, 255);
    for (struct xy xy = XY(0, 0); xy.y < 0x8000; xy.y += 8) {
        for (xy.x = 0; xy.x < 0x8000; xy.x += 8) {
            uint8_t tile_attr = 0xFF; // default
            struct ap_screen * screen = map_screens[XYMAPSCREEN(xy)];
            if (screen != NULL) {
                tile_attr = ap_map_attr(xy);
            }
            fputc(tile_attr, mapf);
        }
    }
    fclose(mapf);
    LOG("Exported map_attrs.pgm");

    FILE * screenf = fopen("screens.txt", "w");
    for (size_t i = 0; i < sizeof(map_screens) / sizeof(*map_screens); i++) {
        struct ap_screen * screen = map_screens[i];
        if (screen == NULL)
            continue;
        fprintf(screenf, "[screen %s]\n", screen->name);
        for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
            struct ap_node n;
            n.tl = XYOP2(node->tl, -, screen->tl);
            n.br = XYOP2(node->br, -, screen->tl);
            fprintf(screenf, "    node: %s " PRIBBWH " ", node->name, PRIBBWHF(n));
            if (node->type == NODE_TRANSITION) {
                if (node->adjacent_node) {
                    fprintf(screenf, "to screen %s node %s", node->adjacent_node->screen->name, node->adjacent_node->name);
                } else {
                    fprintf(screenf, "unmapped");
                }
            }
            fprintf(screenf, "\n");
        }
    }
    fclose(screenf);
    LOG("Exported screens.txt");
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
    struct xy link = ap_link_xy();
    return ap_pathfind_global(link, node, true);
}

static struct ap_node *
ap_screen_commit_node(struct ap_screen * screen, struct ap_node ** new_node_p)
{
    struct ap_node * new_node = *new_node_p;
    NONNULL(new_node);

    if (new_node->tl.x == 0 && new_node->tl.y == 0)
        return NULL;
    assert(new_node->type != NODE_NONE);

    // Check if the node already exists
    for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
        if (node->type != new_node->type && node->type != NODE_ITEM)
            continue;
        if (node->adjacent_direction != new_node->adjacent_direction)
            continue;
        if (!XYIN(new_node->tl, node->tl, node->br) || !XYIN(new_node->br, node->tl, node->br))
            continue;
        //assert(node->adjacent_screen == new_node->adjacent_screen);
        assert_bp(node->tile_attr == new_node->tile_attr);

        //LOG("duplicate node %s == %s", node->name, new_node->name);

        // Merge in new information???
        if (!XYEQ(node->tl, new_node->tl) || !XYEQ(node->br, new_node->br))
            LOG("node tl/br changed: before: " PRIBBV " after: " PRIBBV, PRIBBVF(*node), PRIBBVF(*new_node));

        node->tl = new_node->tl;
        node->br = new_node->br;

        memset(new_node, 0, sizeof *new_node);
        return node;
    }

    // Attach to screen
    new_node->screen = screen;
    LL_PUSH(screen->node_list, new_node);
    LOG("attached node %s", new_node->name);

    // Add goals
    switch (new_node->type) {
    case NODE_TRANSITION:
        if (new_node->adjacent_node == NULL)
            ap_goal_add(GOAL_EXPLORE, new_node);
        break;
    case NODE_ITEM:
        ap_goal_add(GOAL_ITEM, new_node);
        break;
    case NODE_SWITCH:
        ap_goal_add(GOAL_EXPLORE, new_node);
        break;
    case NODE_NONE:
    default:
        ;
    }

    *new_node_p = NONNULL(calloc(1, sizeof **new_node_p));
    return new_node;
}

struct ap_screen *
ap_update_map_screen(bool force)
{
    struct xy tl, br;
    ap_map_bounds(&tl, &br);
    size_t index = XYMAPSCREEN(tl);
    if (index == last_map_index && !force) {
        return NONNULL(map_screens[index]);
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

    uint16_t walk_mask = TILE_ATTR_WALK;
    const uint8_t dirs[6] = {
        1, 2, 3, 4,
        4, 3,
    };
    const struct xy xy_init[6] = {
        tl, XY(tl.x, br.y & ~7), tl, XY(br.x & ~7, tl.y),
        XY(tl.x | 0x1FF, tl.y), XY(tl.x | 0x200, tl.y),
    };
    const struct xy xy_step[6] = {
        dir_dxy[DIR_R], dir_dxy[DIR_R], dir_dxy[DIR_D], dir_dxy[DIR_D],
        dir_dxy[DIR_D], dir_dxy[DIR_D],
    };
    const uint8_t dircount = (tl.x >= 0x4000) ? 6 : 4;
    //uint8_t f = *ap_ram.in_building ? 6 : 2;
    uint8_t f = 2;

    for (uint8_t k = 0; k < dircount; k++) {
        uint8_t i = dirs[k];
        struct xy new_start = XY(0, 0);
        struct xy new_end = XY(0, 0);
        size_t size = 0;

        for (struct xy xy = xy_init[k]; XYIN(xy, tl, br); xy = XYOP2(xy, +, XYOP1(xy_step[k], * 8))) {
            if (xy.x >= 0x4000 && ap_map_attr(xy) == 0x00) { // inside, border 0 is just empty
                continue;
            }
            bool can_walk = true;
            for (int8_t j = 0; j < f + 2; j++) {
                struct xy lxy = XYOP2(xy, -, XYOP1(dir_dxy[i], * 8 * j));
                if (!(ap_tile_attrs[ap_map_attr(lxy)] & walk_mask)) {
                    can_walk = false;
                    break;
                }
            }
            if (can_walk) {
                new_end = XYOP2(xy, -, XYOP1(dir_dxy[i], * 8 * (f + 1)));
                for (int8_t j = f + 1; j < f + 8; j++) {
                    struct xy lxy = XYOP2(new_end, -, XYOP1(dir_dxy[i], * 8));
                    if ((ap_map_attr(lxy) & 0xE0) != 0x80)
                        break;
                    new_end = lxy;
                }
                if (size == 0) {
                    //new_start = XYOP2(xy, -, XYOP1(dir_dxy[i], * 8 * f));
                    new_start = xy;
                }
                size++;
            } else if (size < 2) {
                new_start = new_end = XY(0, 0);
                size = 0;
            } else {
                new_node->tl = XYFN2(MIN, new_start, new_end);
                new_node->br = XYFN2(MAX, new_start, new_end);
                new_node->br = XYOP1(new_node->br, + 7);
                new_node->type = NODE_TRANSITION;
                new_node->adjacent_direction = i;
                new_node->tile_attr = 0; // ap_map_attr(new_node->tl);
                //new_node->adjacent_screen = &map_screens[XYMAPSCREEN(XYOP2(new_node->tl, +, XYOP1(dir_dxy[i], * 0x100)))];
                snprintf(new_node->name, sizeof new_node->name, "0x%02zX %s %d", index, dir_names[i], ++new_node_count);
                ap_screen_commit_node(screen, &new_node);
                new_start = new_end = XY(0, 0);
                size = 0;
            }
        }
        if (size >= 2) {
            new_node->tl = XYFN2(MIN, new_start, new_end);
            new_node->br = XYFN2(MAX, new_start, new_end);
            new_node->br = XYOP1(new_node->br, + 7);
            new_node->type = NODE_TRANSITION;
            new_node->adjacent_direction = i;
            new_node->tile_attr = 0; // not quite true
            //new_node->adjacent_screen = &map_screens[XYMAPSCREEN(XYOP2(new_node->tl, +, XYOP1(dir_dxy[i], * 0x100)))];
            snprintf(new_node->name, sizeof new_node->name, "0x%02zX %s %d", index, dir_names[i], ++new_node_count);
            ap_screen_commit_node(screen, &new_node);
        }
    }
    if (!*ap_ram.in_building) {
        //LOG("xo: %04x, xm: %04x, yo: %04x, ym: %04x", *ap_ram.map_x_offset, *ap_ram.map_x_mask, *ap_ram.map_y_offset, *ap_ram.map_y_mask);
        for (size_t i = 0; i < 0x81; i++) {
            if (ap_ram.over_ent_areas[i] != *ap_ram.map_area)
                continue;

            uint16_t id = ap_ram.over_ent_ids[i];
            if (id > 0x85) {
                LOG("weird id: %u %zu", id, i);
                continue;
            }

            new_node->tl = ap_map16_to_xy(tl, ap_ram.over_ent_map16s[i]);
            new_node->tl.y += 16;
            new_node->br = XYOP1(new_node->tl, + 15);
            new_node->type = NODE_TRANSITION;
            new_node->tile_attr = 0x80; // not quite true
            struct xy entrance = XY(ap_ram.entrance_xs[id], ap_ram.entrance_ys[id]);
            //new_node->adjacent_screen = &map_screens[XYMAPSCREEN(entrance)];
            new_node->adjacent_direction = DIR_U;
            snprintf(new_node->name, sizeof new_node->name, "door 0x%02x", ap_ram.over_ent_ids[i]);
            //LOG("tl: " PRIXYV ", out: " PRIXYV ", map16: %04x", PRIXYVF(tl), PRIXYVF(new_node->tl), ap_ram.over_ent_map16s[i]);
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
            if (ap_tile_attrs[attr] & TILE_ATTR_NODE) {
                struct xy ds[4] = {XY(8, 8), XY(-8, -8), XY(-8, 8), XY(8, -8)};
                new_node->tl = new_node->br = XY(0, 0);
                for (int i = 0; i < 4; i++) {
                    struct xy xy2 = XYOP2(xy, +, ds[i]);
                    if (ap_map_attr(xy2) == attr) {
                        new_node->tl = XYFN2(MIN, xy, xy2);
                        new_node->br = XYFN2(MAX, xy, xy2);
                        new_node->br = XYOP1(new_node->br, + 7);
                        break;
                    }
                }
                if (XYEQ(new_node->tl, new_node->br)) {
                    continue;
                }
                if (ap_tile_attrs[attr] & TILE_ATTR_CHST) {
                    // shift chests down to the square below
                    // XXX this logic should be in ap_plan
                    new_node->tl.y += 16;
                    new_node->br.y += 16;
                }
                new_node->tile_attr = attr;
                const char * attr_name = ap_tile_attr_name(attr);
                if (ap_tile_attrs[attr] & TILE_ATTR_DOOR) {
                    if (attr == 0x5e || attr == 0x5f) {
                        new_node->tl.y += 24;
                        new_node->br.y += 24;
                    } else {
                        new_node->tile_attr = attr;
                    }
                    new_node->type = NODE_TRANSITION;
                    new_node->adjacent_direction = DIR_U;
                    snprintf(new_node->name, sizeof new_node->name, "stairs 0x%02x %s", attr, attr_name);
                } else if (ap_tile_attrs[attr] & TILE_ATTR_CHST) {
                    LOG("FOUND CHEST");
                    new_node->type = NODE_ITEM;
                    snprintf(new_node->name, sizeof new_node->name, "chest 0x%02x %s", attr, attr_name);
                } else if (ap_tile_attrs[attr] & TILE_ATTR_SWCH) {
                    new_node->type = NODE_SWITCH;
                    snprintf(new_node->name, sizeof new_node->name, "switch 0x%02x %s", attr, attr_name);
                } else if (ap_tile_attrs[attr] & TILE_ATTR_LFT0) {
                    new_node->type = NODE_ITEM;
                    snprintf(new_node->name, sizeof new_node->name, "pot 0x%02x %s", attr, attr_name);
                } else {
                    new_node->type = NODE_ITEM;
                    snprintf(new_node->name, sizeof new_node->name, "unknown!? 0x%02x %s", attr, attr_name);
                }
                ap_screen_commit_node(screen, &new_node);
                new_node_count++;
            }
        }
    }
    for (uint8_t i = 0; i < 16; i++) {
        if (ap_ram.sprite_type[i] == 0)
            continue;
        uint16_t sprite_attrs = ap_sprite_attrs[ap_ram.sprite_type[i]];
        if (!(sprite_attrs & (SPRITE_ATTR_SWCH | SPRITE_ATTR_TALK | SPRITE_ATTR_FLLW | SPRITE_ATTR_ITEM)))
            continue;

        struct xy sprite_tl = ap_sprite_xy(i);
        LOG("Sprite %-2u: attrs=%s type=%#x st1=%#x st2=%#x " PRIXYV,
            i, ap_sprite_attr_name(ap_ram.sprite_type[i]), 
            ap_ram.sprite_type[i], ap_ram.sprite_subtype1[i], ap_ram.sprite_subtype2[i],
            PRIXYVF(sprite_tl));
    }

    ap_print_map_screen(screen);
    printf("found %d nodes\n", new_node_count);
    int i = 0; 
    for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
        bool reachable = ap_pathfind_local(screen, ap_link_xy(), node->tl, node->br, false) > 0;
        node->_reachable = reachable;
        printf("   %d. [%c%c] (" PRIXY " x " PRIXY ") %s\n",
                i++, "uR"[reachable], "uM"[node->adjacent_node != NULL], PRIXYF(node->tl), PRIXYF(node->br), node->name);
    }

    return screen;
}

int
ap_map_record_transition_from(struct ap_node * src_node)
{
    if (src_node->adjacent_node != NULL)
        return 0;

    struct ap_screen * src_screen = src_node->screen;

    struct xy dst_xy = ap_link_xy();
    struct ap_screen * dst_screen = map_screens[XYMAPSCREEN(dst_xy)];
    assert(dst_screen != src_screen);

    // Find dst_node
    struct ap_node * dst_node = NULL;
    for (struct ap_node * node = dst_screen->node_list->next; node != dst_screen->node_list; node = node->next) {
        if (node->type != NODE_TRANSITION)
            continue;
        if (!XYEQ(dir_dxy[node->adjacent_direction], XYOP1(dir_dxy[src_node->adjacent_direction], * -1)))
            continue;
        if (!XYIN(dst_xy, node->tl, node->br)) 
            continue;
        dst_node = node;
        break;
    }
    if (dst_node == NULL) {
        LOG("Warning! dst_node is NULL so making own node");
        static struct ap_node * new_node = NULL;
        if (new_node == NULL)
            new_node = NONNULL(calloc(1, sizeof *new_node));

        // TODO: Should be based on the size of src_node?
        new_node->tl = XYOP1(dst_xy, & ~7);
        new_node->br = XYOP1(new_node->tl, + 15);
        new_node->type = NODE_TRANSITION;
        if (src_node->tile_attr == 0x5e || src_node->tile_attr == 0x5f) {
            new_node->adjacent_direction = DIR_U;
            new_node->tile_attr = src_node->tile_attr ^ 0x1; // XXX
        } else {
            new_node->adjacent_direction = dir_opp[src_node->adjacent_direction];
        }
        snprintf(new_node->name, sizeof new_node->name, "0x%02X %s cross", XYMAPSCREEN(dst_xy), dir_names[new_node->adjacent_direction]);

        dst_node = ap_screen_commit_node(dst_screen, &new_node);
    }
    assert(dst_node != NULL);

    src_node->adjacent_node = dst_node;
    dst_node->adjacent_node = src_node;

    return 0;
}
