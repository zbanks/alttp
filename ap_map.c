#include <string.h>
#include "ap_map.h"
#include "ap_math.h"
#include "ap_macro.h"
#include "ap_snes.h"
#include "ap_plan.h"
#include "pq.h"
#include "pm.h"

static struct ap_screen * map_screens[0x100 * 0x100];
static bool map_screen_mask_x[0x100];
static bool map_screen_mask_y[0x100];
static size_t last_map_index = -1;

static struct ap_target {
    struct xy tl;
    uint16_t joypad;
    uint16_t joypad_mask;
} ap_targets[2048];
static size_t ap_target_count = 0;
static int ap_target_timeout = 0;
static int ap_target_subtimeout = 0;
static bool ap_target_scripted = false;
static struct xy ap_target_dst_tl;
static struct xy ap_target_dst_br;
static struct ap_screen * ap_target_screen;

const char * const ap_node_type_names[] = {
#define X(type) [CONCAT(NODE_, type)] = #type,
NODE_TYPE_LIST
#undef X
};

static const struct ap_script ap_scripts[] = {
    {
        .start_tl = XY(0x9af8, 0x21c0),
        .start_item = -1,
        .sequence = "^^<^v>>^<^",
        .type = SCRIPT_SEQUENCE,
        .name = "Dam Chest Block Puzzle",
    },
    {
        //.start_tl = XY(0xab00, 0x2370),
        .start_tl = XY(0xaaf0, 0x2340),
        .start_item = INVENTORY_BOMBS,
        .sequence = "Yvvv>v<vv^^>>>vv<UA>^^<<<^<<vvv>vv>>>>>",
        .type = SCRIPT_SEQUENCE,
        .name = "Blind's House Block Puzzle",
    },
    {
        .start_tl = XY(0x0988, 0x0c80),
        .start_item = INVENTORY_BOMBS,
        .sequence = "Yvvvvvvvvvvv",
        .type = SCRIPT_SEQUENCE,
        .name = "Bomb wall below Link's House",
    },
    {
        .start_tl = XY(0x0068, 0x0990),
        .start_item = INVENTORY_BOMBS,
        .sequence = "Y>>><<<>>><<<>>><<<>>>",
        .type = SCRIPT_SEQUENCE,
        .name = "Bombable Hut in Kak",
    },
    {
        .start_tl = XY(0x52f8, 0x0e60),
        .start_item = -1,
        .sequence = NULL,
        .type = SCRIPT_KILLALL,
        .name = "HC kill guard for key",
    },
    {
        .start_tl = XY(0x4878, 0x0fb0),
        .start_item = -1,
        .sequence = NULL,
        .type = SCRIPT_KILLALL,
        .name = "HC kill green guard for doors",
    },
    {
        .start_tl = XY(0x4950, 0x0f80),
        .start_item = -1,
        .sequence = NULL,
        .type = SCRIPT_KILLALL,
        .name = "HC kill blue guard for door",
    },
    {
        .start_tl = XY(0x4250, 0x1060),
        .start_item = -1,
        .sequence = NULL,
        .type = SCRIPT_KILLALL,
        .name = "HC kill miniboss free Zelda",
    },
};

static bool add_explore_goals_global = true;
static const struct ap_screen_info {
    uint16_t id;
    char name[40];
    bool add_explore_goals;
    bool key_doors;
} ap_screen_infos[] = {
    // Light Overworld
    { .id = 0x0402, .name = "North of Kak", },
    { .id = 0x0406, .name = "Sanctuary Yard", },
    { .id = 0x0408, .name = "Graveyard", },
    { .id = 0x040c, .name = "Witches' Yard", },
    { .id = 0x0606, .name = "Castle Yard", },
    { .id = 0x060c, .name = "Ruins", },
    { .id = 0x0600, .name = "Kak Village", .add_explore_goals = true, },
    { .id = 0x0804, .name = "Smith's Yard", .add_explore_goals = true, },
    { .id = 0x0a00, .name = "Maze Race" },
    { .id = 0x0a02, .name = "Library Yard", .add_explore_goals = true, },
    { .id = 0x0a04, .name = "Flute Grove", },
    { .id = 0x0a08, .name = "Link's Yard", },
    { .id = 0x0a68, .name = "Well Uncle", },
    { .id = 0x0b68, .name = "Well Chest", },
    { .id = 0x0c0a, .name = "Fortune Shore", },
    // Overworld Interiors
    { .id = 0x2161, .name = "Link's House", },
    { .id = 0x2178, .name = "Library", },
    { .id = 0x2188, .name = "Witches' Hut", },
    { .id = 0x2198, .name = "Dam Chest", },
    { .id = 0x2550, .name = "Fortune Teller", },
    { .id = 0x1d58, .name = "Bat Cave Exit", },
    { .id = 0x2548, .name = "Smiths' House", },
    { .id = 0x1f68, .name = "Maze Race Entrance", },
    { .id = 0x2388, .name = "Blind's House", .add_explore_goals = true, },
    { .id = 0x23a8, .name = "Blind's Basement", .add_explore_goals = true, },
    { .id = 0x22a8, .name = "Blind's Storage",},
    // Hyrule Castle
    { .id = 0x0c48, .name = "HC Entrance", },
    { .id = 0x0e50, .name = "HC First Key", .key_doors = true, .add_explore_goals = true},
    { .id = 0x0f50, .name = "HC Walkway"},
    { .id = 0x0f48, .name = "HC Green Guard"},
    { .id = 0x1040, .name = "HC Zelda Jail"},
    // Eastern Palace
    { .id = 0x1988, .name = "EP Entrance", },
    { .id = 0x1888, .name = "EP Hall 1", },
    { .id = 0x1688, .name = "EP Dodgeball", },
    { .id = 0x1488, .name = "EP Bigchest", },
    { .id = 0x1490, .name = "EP Catwalk", },
    { .id = 0x1491, .name = "EP 5 pots", },
    { .id = 0x1591, .name = "EP Chest&Ledge", },
    { .id = 0x1481, .name = "EP Crossover", },
    { .id = 0x1580, .name = "EP Stalfos", },
    { .id = -1 },
};


struct point_state {
    // Parts that need to be cleared each time this ap_pathfind_local is called
    struct point_state * from;
    uint32_t gscore;
    uint32_t fscore;

    // Parts that can be cached for the same (screen, frame #)
    struct xy xy;
    uint16_t tile_attrs;
    uint8_t raw_tile;
    uint8_t ledge;
    uint8_t corner;
    uint32_t cost;
};

static int
ap_pathfind_local(struct ap_screen * screen, struct xy start_xy, struct xy destination_tl, struct xy destination_br, bool commit);
static void
ap_print_pathfind_local_state(struct point_state *buf, struct xy grid);

static struct xy
ap_box_edge(struct xy tl, struct xy br, int dir) {
    // Step `count` pixels in direction `dir` from the bounding box `tl,br`
    struct xy dxy = dir_dxy[dir];
    return XY(
        ((dir_dx[dir] <= 0) ? tl.x : br.x),
        ((dir_dy[dir] <= 0) ? tl.y : br.y));
}

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

static void
ap_map_room_bounds(struct xy base, struct xy * tl, struct xy * br) {
    // Only works in current "room"
    uint8_t layout = *ap_ram.room_layout;
    struct xy size = XY(0x200, 0x200);
    bool in_right = (base.x & 0x100) != 0;
    bool in_bottom = (base.y & 0x100) != 0;
    switch (layout >> 2) {
    case ROOM_LAYOUT_A_B_C_D: size = XY(0x100, 0x100); break;
    case ROOM_LAYOUT_AC_BD: size = XY(0x100, 0x200); break;
    case ROOM_LAYOUT_AB_CD: size = XY(0x200, 0x100); break;
    case ROOM_LAYOUT_ABCD: size = XY(0x200, 0x200); break;
    case ROOM_LAYOUT_A_BD_C: size = XY(0x100, (in_right) ? 0x200 : 0x100); break;
    case ROOM_LAYOUT_AC_B_D: size = XY(0x100, (in_right) ? 0x100 : 0x200); break;
    case ROOM_LAYOUT_A_B_CD: size = XY((in_bottom) ? 0x200 : 0x100, 0x100); break;
    case ROOM_LAYOUT_AB_C_D: size = XY((in_bottom) ? 0x100 : 0x200, 0x100); break;
    default: assert_bp(false);
    }
    size = XYOP1(size, - 1);
    struct xy mask = XY(~size.x, ~size.y);
    *tl = XYOP2(base, &, mask);
    *br = XYOP2(*tl, +, size);
}

void
ap_map_bounds(struct xy * topleft, struct xy * bottomright)
{
    if (*ap_ram.in_building) {
        struct xy link = ap_link_xy();
        link.y = CAST16(link.y - 8);
        ap_map_room_bounds(link, topleft, bottomright);
    } else {
        topleft->x = (uint16_t) (*ap_ram.map_x_offset * 8);
        topleft->y = (uint16_t) (*ap_ram.map_y_offset);
        bottomright->x = (uint16_t) (topleft->x + ((*ap_ram.map_x_mask * 8) | 0xF));
        bottomright->y = (uint16_t) (topleft->y + (*ap_ram.map_y_mask | 0xF));
    }
}

uint16_t
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
        assert(XYINDOORS(point));
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

struct xy
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
ap_print_map_screen_pair() {
    struct xy link = ap_link_xy();
    ap_print_map_screen(map_screens[XYMAPSCREEN(link)]);
    if (XYINDOORS(link)) {
        ap_print_map_screen(map_screens[XYMAPSCREEN(XYFLIPBG(link))]);
    }
}

void
ap_print_map_screen(struct ap_screen * screen)
{
    FILE * mapf = NONNULL(fopen("map", "w"));
    FILE * mapimg = NONNULL(fopen("map.pbm.tmp", "w"));

    struct xy link = ap_link_xy();
    struct xy link_tl = link;
    struct xy link_br = XYOP1(link, + 15);
    if (screen == NULL) {
        screen = map_screens[XYMAPSCREEN(link_tl)];
    }

    struct xy screen_size = XYOP1(XYOP2(screen->br, -, screen->tl), / 8 + 1);
    fprintf(mapimg, "P6 %u %u %u\n", screen_size.x, screen_size.y, 255);

    uint16_t semi_tile_attrs = TILE_ATTR_LFT0 | TILE_ATTR_DOOR;
    bool inside = XYINDOORS(screen->tl);

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
                if (ap_sprites[i].type == 0) {
                    continue;
                }
                if (XYIN(xy, ap_sprites[i].tl, ap_sprites[i].br) ||
                    (inside && XYIN(XY(xy.x ^ 0x200, xy.y), ap_sprites[i].tl, ap_sprites[i].br))) {
                    if ((xy.x ^ xy.y) & 0x8) {
                        fprintf(mapf, TERM_MAGENTA("%02x "), tile_attr);
                    } else {
                        fprintf(mapf, TERM_MAGENTA(TERM_BOLD("%02x ")), ap_sprites[i].type);
                    }
                    fprintf(mapimg, "%c%c%c", 255, px_shade / 4, 255);
                    goto next_point;
                }
            }
            for (struct ap_goal * goal = ap_goal_list->next; goal != ap_goal_list; goal = goal->next) {
                if (goal->node == NULL) {
                    continue;
                }
                if (XYIN(xy, goal->node->locked_xy, XYOP1(goal->node->locked_xy, +7))) {
                    fprintf(mapf, TERM_CYAN(TERM_BOLD("%02x ")), tile_attr);
                    fprintf(mapimg, "%c%c%c", 64, 64, 0);
                    goto next_point;
                }
                if (XYIN(xy, goal->node->tl, goal->node->br)) {
                    fprintf(mapf, TERM_YELLOW("%02x "), tile_attr);
                    fprintf(mapimg, "%c%c%c", 255, 255 - goal->attempts * 64, 0);
                    goto next_point;
                }
            }
            for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
                if (XYIN(xy, node->locked_xy, XYOP1(node->locked_xy, +7))) {
                    fprintf(mapf, TERM_CYAN(TERM_BOLD("%02x ")), tile_attr);
                    fprintf(mapimg, "%c%c%c", 64, 64, 0);
                    goto next_point;
                }
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
                if (XYIN(xy, ap_targets[i].tl, XYOP1(ap_targets[i].tl, +8))) {
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

    ap_targets[count] = (struct ap_target) { .tl = ap_link_xy()};
    //fprintf(stderr, "(dest) ");
    for (size_t i = 0; i < count; i++) {
        if (XYEQ(ap_targets[i].tl, XY(0,0))) {
            LOG("target 0");
            assert_bp(false);
        }
        ap_target_timeout += XYL1DIST(ap_targets[i].tl, ap_targets[i+1].tl) * 2;
        if (ap_targets[i].joypad != ap_targets[i+1].joypad) {
            ap_target_timeout += 1;
        }
        //fprintf(stderr, PRIXYV ", ", PRIXYVF(ap_targets[i]));
    }
    //fprintf(stderr, PRIXYV " (link)\n", PRIXYVF(ap_targets[count]));

    for (size_t i = count; i < sizeof(ap_targets) / sizeof(*ap_targets); i++) {
        ap_targets[count] = (struct ap_target) {.tl = XY(0, 0), .joypad = 0, .joypad_mask = 0};
    }

    ap_target_subtimeout = 32;
    ap_target_timeout += 64;
    fflush(NULL);
    LOG("Following %zu targets with timeout: %d", ap_target_count, ap_target_timeout);
    ap_print_map_screen(NULL);
    return ap_target_timeout;
}

int
ap_follow_targets(uint16_t * joypad)
{
    static struct xy last_link = XY(0, 0);
    static int stationary_link_count = 0;
    struct xy link = ap_link_xy();

    if (XYEQ(last_link, link)) {
        stationary_link_count++;
    } else {
        stationary_link_count = 0;
    }
    last_link = link;

    if (stationary_link_count >= 128) {
        LOGB("Stuck Link Detected! " PRIXY " en route to " PRIXY,
            PRIXYF(link), PRIXYF(ap_targets[0].tl));
        ap_target_timeout = 0;
        stationary_link_count = 0;
    }

    if (ap_target_timeout <= 0) {
        //INFO("L:" PRIXY "; " PRIXY " timeout", PRIXYF(link), PRIXYF(ap_targets[0].tl));
        LOG("L:" PRIXY "; " PRIXY " timeout", PRIXYF(link), PRIXYF(ap_targets[0].tl));
        ap_target_count = 0;
        return RC_FAIL;
    }

    if (!ap_target_scripted && (ap_target_subtimeout-- <= 0 || ap_sprites_changed)) {
        int prev_timeout = ap_target_timeout;
        int rc = ap_pathfind_local(NULL, link, ap_target_dst_tl, ap_target_dst_br, true);
        assert_bp(rc >= 0);
        ap_target_timeout = MIN(prev_timeout, ap_target_timeout);
    }

    struct ap_target target;
    uint16_t joypad_mask = 0;
    while (true) {
        if (ap_target_count < 1) {
            //INFO("L:" PRIXY "; done", PRIXYF(link));
            //LOG("L:" PRIXY "; done", PRIXYF(link));
            return RC_DONE;
        }

        target = ap_targets[ap_target_count-1];
        if (target.joypad_mask & joypad_mask) {
            break;
        }

        *joypad &= (uint16_t) ~target.joypad_mask;
        *joypad |= target.joypad;
        joypad_mask |= target.joypad_mask;

        if (XYL1DIST(link, target.tl) <= 1) {
            ap_target_count--;
            ap_target_subtimeout = 32;
            continue;
        }
        if (!ap_target_scripted && ap_target_count > 1) {
            struct ap_target next_target = ap_targets[ap_target_count-2];
            if (XYIN(link, XYFN2(MIN, target.tl, next_target.tl), XYFN2(MAX, target.tl, next_target.tl))) {
                ap_target_count--;
                ap_target_subtimeout = 32;
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

    //INFO("L:" PRIXY "; %zu:" PRIXY "; %d %#x", PRIXYF(link), ap_target_count, PRIXYF(ap_targets[ap_target_count-1]), ap_target_timeout, *ap_ram.push_dir_bitmask);
    //LOG("L:" PRIXY "; %zu:" PRIXY "; %d", PRIXYF(link), ap_target_count, PRIXYF(ap_targets[ap_target_count-1]), ap_target_timeout);

    if (link.x > target.tl.x)
        JOYPAD_SET(LEFT);
    else if (link.x < target.tl.x)
        JOYPAD_SET(RIGHT);
    if (link.y > target.tl.y)
        JOYPAD_SET(UP);
    else if (link.y < target.tl.y)
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

    uint8_t d = (*ap_ram.link_direction / 2) + 1;
    struct xy sword_up = XYOP1(dir_dxy[d], * 16 + 8);
    //struct xy sword_rt = XYOP1(dir_dxy[dir_cw[d]], * 16 + 8);
    struct xy link_sword_up = XYOP2(link, +, sword_up);
    //struct xy link_sword_rt = XYOP2(link, +, sword_rt);

    for (uint8_t i = 0; i < 16; i++) {
        if (!*ap_ram.inventory_sword) {
            break;
        }
        if (!ap_sprites[i].active) {
            continue;
        }
        if (ap_sprites[i].attrs & SPRITE_ATTR_ENMY) {
            if (XYL1BOXDIST(link_sword_up, ap_sprites[i].tl, ap_sprites[i].br) <= 8) {
                JOYPAD_MASH(B, 0x10); // Sword
                break;
            }
        }
    }

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
    if (XYINDOORS(src) && XYINDOORS(dst_tl) && XYINDOORS(dst_br)) {
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
    if (commit) {
        ap_set_targets(0);
        ap_update_map_screen(true);
    }

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

    static struct point_state buf[0x82 * 0x82]; 

    // CORNER is used to identify boundaries of 2x2 tiles (like bushes)
    enum corner {
        CORNER_NONE = 0,
        CORNER_TL,
        CORNER_TR,
        CORNER_BL,
        CORNER_BR,
        CORNER_TA, // Left adjacent square
        CORNER_BA,
        CORNER_AA, // Up-Left diagonally adjacent
    };
    if (grid.x * grid.y > 0x80 * 0x80) {
        printf("error: grid too large: " PRIXY "\n", PRIXYF(grid));
        return -1;
    }

    // Make a state[y][x]
    struct point_state (*state)[grid.x + 2] = (void *) &buf[0x83];

    // Check if cache is valid
    static struct ap_screen *last_screen = NULL;
    static uint32_t last_frame = 0;
    if (last_screen == screen && last_frame == ap_frame) {
        for (uint16_t y = 0; y < grid.y; y++) {
            for (uint16_t x = 0; x < grid.x; x++) {
                assert_bp(&state[y][x] == &buf[0x83 + (grid.x + 2) * y + x]);
                state[y][x].from = NULL;
                state[y][x].gscore = (uint32_t) -1;
                state[y][x].fscore = (uint32_t) -1;
            }
        }
    } else {
        last_screen = screen;
        last_frame = ap_frame;

        for (size_t i = 0; i < 0x82 * 0x82; i++) {
            buf[i].ledge = 0xFF;
        } 

        for (uint16_t y = 0; y < grid.y; y++) {
            for (uint16_t x = 0; x < grid.x; x++) {
                assert_bp(&state[y][x] == &buf[0x83 + (grid.x + 2) * y + x]);
                state[y][x].from = NULL;
                state[y][x].xy = XYOP1(XYOP2(XY(x, y), +, tl_offset), * 8);
                state[y][x].gscore = (uint32_t) -1;
                state[y][x].fscore = (uint32_t) -1;
                state[y][x].cost = 0;
                state[y][x].corner = CORNER_NONE;
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
                //if (tile & (TILE_ATTR_WALK)) {
                    cost = 0;
                } else if (tile & TILE_ATTR_LDGE) {
                    cost = (1 << 20); // calculated at search time
                    //cost = 0;
                    assert_bp((raw_tile &~0x7) == 0x28);
                    ledge_mask = 1ul << (raw_tile & 0x7);
                } else if (tile & TILE_ATTR_LFT0) {
                    cost = 40;
                    if (state[y][x].corner == CORNER_NONE)  {
                        state[y][x].corner = CORNER_TL;
                        state[y][x+1].corner = CORNER_TR;
                        state[y+1][x].corner = CORNER_BL;
                        state[y+1][x+1].corner = CORNER_BR;
                        if (state[y][x-1].corner == CORNER_NONE) {
                            state[y][x-1].corner = CORNER_TA;
                            state[y+1][x-1].corner = CORNER_BA;
                        }
                        if (state[y-1][x-1].corner == CORNER_NONE) {
                            state[y-1][x-1].corner = CORNER_AA;
                        }
                    }
                } else if (XYIN(mapxy, destination_tl, destination_br)) {
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
        for (uint8_t i = 0; i < 16; i++) {
            if (ap_sprites[i].type == 0) {
                continue;
            }
            if (!XYIN(ap_sprites[i].hitbox_tl, screen->tl, screen->br)) {
                continue;
            }
            if (ap_sprites[i].attrs & SPRITE_ATTR_ENMY && 
                ap_sprites[i].state != 0x00) { // dead
                struct xy center = XYOP1(XYOP2(ap_sprites[i].hitbox_tl, -, screen->tl), / 8);
                for (int dx = -4; dx <= 4; dx++) {
                    for (int dy = -4; dy <= 4; dy++) {
                        struct xy ds = XYOP2(center, +, XY(dx, dy));
                        if (!XYUNDER(ds, grid)) {
                            continue;
                        }
                        state[ds.y][ds.x].cost += MAX(0, 7 - (ABS(dx) + ABS(dy))) * 100;
                    }
                }
            }
            if ((ap_sprites[i].attrs & (SPRITE_ATTR_BLKF | SPRITE_ATTR_BLKS)) &&
                ap_sprites[i].state != 0x0A && // carried
                ap_sprites[i].state != 0x06) { // thrown
                assert_bp(ap_sprites[i].state == 0x08 || 
                        ap_sprites[i].state == 0x09 || 
                        ap_sprites[i].state == 0x00);
                //volatile struct xy center = XYOP1(XYOP2(ap_sprites[i].hitbox_tl, -, screen->tl), / 8);
                //assert_bp(i != 11);
                struct xy hb_tl = XYOP2(ap_sprites[i].hitbox_tl, -, screen->tl);
                struct xy hb_br= XYOP2(ap_sprites[i].hitbox_br, -, screen->tl);
                hb_tl = XYOP1(hb_tl, / 8);
                hb_br = XYOP1(hb_br, / 8);
                hb_tl = XYOP1(hb_tl, -1);
                hb_br = XYOP1(hb_br, +1);
                /*
                if (i == 11) {
                    LOG(TERM_BOLD("11:") " " PRIXYV, PRIXYVF(center));
                }
                */
                struct xy ds;
                for (ds.x = hb_tl.x; ds.x <= hb_br.x; ds.x++) {
                    for (ds.y = hb_tl.y; ds.y <= hb_br.y; ds.y++) {
                        if (!XYUNDER(ds, grid)) {
                            continue;
                        }
                        state[ds.y][ds.x].cost += (1 << 20);
                    }
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
    if (commit) {
        ap_print_pathfind_local_state(buf, grid);
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
            enum corner corner = state[neighbor.y][neighbor.x].corner;
            if (corner != CORNER_NONE) {
                if (i >= 5) continue;
                if ((i == DIR_D || i == DIR_U) && (corner == CORNER_TR || corner == CORNER_BR || corner == CORNER_TA || corner == CORNER_BA))
                    continue;
                if ((i == DIR_L || i == DIR_R) && (corner == CORNER_BL || corner == CORNER_BR))
                    continue;
                if ((i == DIR_D || i == DIR_R) && corner == CORNER_AA)
                    continue;
            }
            /*
            // Handle stairs
            if (state[node.y][node.x].raw_tile == 0x3E ||
                state[node.y][node.x].raw_tile == 0x1E) {
                // FIXME: This needs to change now that we've refactored
                // BG1/BG2  to be different screens
                neighbor.x ^= 0x200 / 8;
                //neighbor = XYOP2(neighbor, +, dir_dxy[i]);
                //neighbor = XYOP2(neighbor, +, dir_dxy[i]);
                assert_bp(XYUNDER(neighbor, grid));
            }
            */
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
    ap_targets[count++] = (struct ap_target) { .tl = XY(
            MIN(next->xy.x, destination_br.x - 15),
            MIN(next->xy.y, destination_br.y - 15)),
        .joypad = 0, .joypad_mask = 0,
    };

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
                ap_targets[count++] = (struct ap_target) { .tl = next->xy };
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
    ap_target_scripted = false;
    ap_print_map_screen(screen);
    return final_gscore;
}

static int
ap_pathfind_node_distance(const struct ap_node * src, const struct ap_node * dst) 
{
    assert(src->screen == dst->screen);
    if (src->type == NODE_NONE) {
        // Fake node; have to use ap_pathfind_local
        return ap_pathfind_local(src->screen, src->tl, dst->tl, dst->br, false);
    }
    assert(dst->type != NODE_NONE);
    assert_bp(src->screen->distances_length > 0);
    const struct ap_node_distance *dists = src->screen->distances;
    for (size_t i = 0; i < src->screen->distances_length; i++) {
        if (dists[i].src == src && dists[i].dst == dst) {
            return dists[i].distance;
        }
    }
    return -1;
}

static int
ap_pathfind_global(struct xy start_xy, struct ap_node * destination, bool commit, int _max_distance)
{
    uint64_t max_distance = UINT64_MAX;
    if (_max_distance > 0) {
        max_distance = (uint64_t) _max_distance;
    }

    struct ap_screen * start_screen = map_screens[XYMAPSCREEN(start_xy)];
    if (start_screen == NULL) {
        LOG("start screen == NULL");
        return -1;
    }
    if (destination->_debug_blocked) {
        return -1;
    }

    static uint64_t iter = 0;
    iter += 2;
    static struct ap_node _start_node;
    struct ap_node * start_node = &_start_node;
    *start_node = (struct ap_node) {
        .screen = start_screen,
        .tl = start_xy,
        .type = NODE_NONE,
        .pgsearch = {
            .iter = iter,
            //.xy = start_xy,
            .from = NULL,
            .distance = 0,
        }
    };

    if (start_screen == destination->screen) {
        int local_dist = ap_pathfind_local(start_screen, start_xy, destination->tl, destination->br, commit);
        if (local_dist >= 0) {
            destination->pgsearch.iter = iter;
            destination->pgsearch.from = start_node;
            return local_dist;
        }
    }

    static struct pq * pq = NULL;
    if (pq == NULL) {
        pq = pq_create(sizeof(struct ap_node *));
        if (pq == NULL) exit(1);
    }
    pq_clear(pq);
    pq_push(pq, 0, &start_node);

    size_t r = 0;
    while (true) {
        r++;
        struct ap_node * node;
        uint64_t distance;
        if (pq_pop(pq, &distance, &node) < 0) 
            goto search_failed;
        if (distance > max_distance)
            goto search_too_far;
        if (node->pgsearch.iter == iter + 1)
            continue;
        if (ap_node_islocked(node))
            continue;
        assert(node->pgsearch.iter == iter);
        node->pgsearch.iter++;
        if (node == destination)
            goto search_done;
        assert(node->type == NODE_TRANSITION || node == start_node);

        // Try going to an adjacent screen
        if (node->adjacent_node != NULL && node->adjacent_direction != 0) {
            uint64_t distance = node->pgsearch.distance + 1000;
            if ((node->adjacent_node->pgsearch.iter == iter &&
                 node->adjacent_node->pgsearch.distance < distance) ||
                 node->adjacent_node->pgsearch.iter < iter) {
                node->adjacent_node->pgsearch = (struct ap_node_pgsearch) {
                    .iter = iter,
                    //.xy = XYMID(node->adjacent_node->tl, node->adjacent_node->br),
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
            if (adj_node->_debug_blocked)
                continue;
            if (!(adj_node == destination || adj_node->type == NODE_TRANSITION))
                continue;
            if (adj_node->pgsearch.iter == iter + 1)
                continue;
            //int delta_distance = ap_pathfind_local(node->screen, node->pgsearch.xy, adj_node->tl, adj_node->br, false);
            int delta_distance = ap_pathfind_node_distance(node, adj_node);
            if (delta_distance < 0)
                continue;

            uint64_t distance = node->pgsearch.distance + delta_distance;
            if ((adj_node->pgsearch.iter == iter &&
                 adj_node->pgsearch.distance > distance) ||
                 adj_node->pgsearch.iter < iter) {
                 adj_node->pgsearch = (struct ap_node_pgsearch) {
                    .iter = iter,
                    //.xy = XYMID(adj_node->tl, adj_node->br),
                    .from = node,
                    .distance = distance,
                };
                pq_push(pq, distance, &adj_node);
            }
        }
    }
search_too_far:
    if (commit) LOG("Search exceeded limit after %zu steps (max_distance=%d)", r, _max_distance);
    return _max_distance + 1;
search_failed:
    if (commit) LOG("Search failed after %zu steps (max_distance=%d)", r, _max_distance);
    return -1;
search_done:
    if (commit) LOG("Search done in %zu steps, pq size = %zu", r, pq_size(pq));
    else return destination->pgsearch.distance;

    struct ap_node * next = destination;
    size_t count = 0;
    while (next != NULL) {
        assert(next->pgsearch.iter == iter + 1);
        LOG("    %zu: %s %ld", count, next->name, next->pgsearch.distance);
        next = next->pgsearch.from;
        count++;
    }

    return destination->pgsearch.distance;
}

void
ap_print_map_graph()
{
    FILE *graphf = NONNULL(fopen("full_map.dot", "w"));
    fprintf(graphf, "digraph map {\nconcatenate=true\n");
    struct xy link = ap_link_xy();
    int32_t link_min_dist = INT32_MAX;
    struct ap_node * link_nearest_node = NULL;

    for (struct xy xy = XY(0, 0); xy.y < 0x8000; xy.y += 0x100) {
        if (!map_screen_mask_y[xy.y / 0x100]) continue;
        for (xy.x = 0; xy.x < 0xC000; xy.x += 0x100) {
            if (!map_screen_mask_x[xy.x / 0x100]) continue;
            struct ap_screen * screen = map_screens[XYMAPSCREEN(xy)];
            if (screen == NULL || !XYEQ(screen->tl, xy)) continue;

            fprintf(graphf, "subgraph cluster_%d { color=black; label=\"%s\"\n", screen->id, screen->name);
            for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
                const char *shape = "oval";
                switch (node->type) {
                case NODE_TRANSITION: shape = "box"; break;
                case NODE_CHEST: shape = "house"; break;
                case NODE_ITEM: shape = "house"; break;
                case NODE_SWITCH: shape = "parallelogram"; break;
                default: break;
                }
                const char *special = "";
                if (node->_debug_blocked) {
                    special = "fillcolor=grey";
                }
                fprintf(graphf, " n%p [label=\"%s\" shape=%s %s]\n", node, node->name, shape, special);
            }
            fprintf(graphf, "}\n");

            for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
                struct xy node_mid = XYMID(node->tl, node->br);
                if (XYL1DIST(link, node_mid) < link_min_dist) {
                    link_min_dist = XYL1DIST(link, node_mid);
                    link_nearest_node = node;
                }

                if (node->adjacent_node != NULL) {
                    fprintf(graphf, " n%p -> n%p [label=%s color=purple]\n", node, node->adjacent_node, dir_names[node->adjacent_direction]);
                }

                for (struct ap_node * node2 = node->next; node2 != screen->node_list; node2 = node2->next) {
                    if (node->type != NODE_TRANSITION && node2->type != NODE_TRANSITION) continue;
                    int dist = ap_pathfind_local(screen, XYMID(node->tl, node->br), node2->tl, node2->br, false);
                    if (dist >= 0) {
                        fprintf(graphf, " n%p -> n%p [color=blue dir=both]\n", node, node2);
                    }
                }
            }
        }
    }

    for (struct ap_goal * goal = ap_goal_list->next; goal != ap_goal_list; goal = goal->next) {
        if (goal->node == NULL) continue;
        fprintf(graphf, " n%p [color=red];\n", goal->node);
    }

    fprintf(graphf, " n%p [color=green];\n", link_nearest_node);
    fprintf(graphf, "}\n");
    fclose(graphf);
    LOG("Exported map graph to full_map.dot");
}

static void
ap_print_pathfind_local_state(struct point_state *buf, struct xy grid) {
    struct point_state (*state)[grid.x + 2] = (void *) &buf[0x83];
    struct xy link = ap_link_xy();
    link = XYOP1(link, & 0xFFF8);

    FILE * mapf = fopen("local_cost.pgm.tmp", "w");
    fprintf(mapf, "P6 %u %u %u\n", grid.x, grid.y, 255);
    for (uint16_t y = 0; y < grid.y; y++) {
        for (uint16_t x = 0; x < grid.x; x++) {
            uint32_t cost = state[y][x].cost;
            uint8_t c = 0;
            if (cost >= (1 << 20)) {
                c = 0;
            } else if (cost >= 10000) {
                c = 25;
            } else if (cost >= 1000) {
                c = 40;
            } else if (cost > 200) {
                c = 50;
            } else {
                c = 255 - cost;
            }
            if (XYEQ(state[y][x].xy, link)) {
                fputc(0, mapf);
                fputc(255, mapf);
                fputc(c, mapf);
            } else {
                fputc(c, mapf);
                fputc(c, mapf);
                fputc(c, mapf);
            }
        }
    }
    fclose(mapf);
    rename("local_cost.pgm.tmp", "local_cost.pgm");
}

void
ap_print_map_full()
{
    ap_print_state();
    ap_print_map_graph();
    FILE * mapf = fopen("full_map.pgm.tmp", "w");
    uint16_t size_x = 0, size_y = 0;
    for (size_t i = 0; i < 0xC0; i++) {
        if (map_screen_mask_x[i]) size_x += 0x100 / 8;
        if (map_screen_mask_y[i]) size_y += 0x100 / 8;
    }
    fprintf(mapf, "P6 %u %u %u\n", size_x, size_y, 255);
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
    for (struct xy xy = XY(0, 0); xy.y < 0x8000; xy.y += 8) {
        if (!map_screen_mask_y[xy.y / 0x100]) continue;
        //for (xy.x = 0; xy.x < 0x8000; xy.x += 8) {
        for (xy.x = 0; xy.x < 0xC000; xy.x += 8) {
            if (!map_screen_mask_x[xy.x / 0x100]) continue;
            //if (xy.x >= 0x1000 && xy.x < 0x4000) continue;
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
                if (XYIN(xy, ap_targets[i].tl, XYOP1(ap_targets[i].tl, +8))) {
                    fputc(0, mapf);
                    fputc(255 - tile_attr, mapf);
                    fputc(255, mapf);
                    goto next_point;
                }
            }
            for (struct ap_goal * goal = ap_goal_list->next; goal != ap_goal_list; goal = goal->next) {
                if (goal->node == NULL) {
                    continue;
                }
                if (XYIN(xy, goal->node->tl, goal->node->br)) {
                    //if (ap_graph_is_blocked(&goal->graph)) {
                    if (!ap_req_is_satisfied(&goal->req)) {
                        fprintf(mapf, "%c%c%c", 200, 0, 100);
                    } else {
                        fprintf(mapf, "%c%c%c", 255, 255 - goal->attempts * 64, 0);
                    }
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
        if (XYMAPSCREEN(screen->tl) != i)
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
ap_pathfind_node(struct ap_node * node, bool commit, int max_distance)
{
    /*
    if (node->adjacent_direction) {
        struct xy offset = XYOP1(dir_dxy[node->adjacent_direction], * -7);
        tl = XYOP2(tl, +, offset);
        br = XYOP2(br, +, offset);
    }
    */
    struct xy link = ap_link_xy();
    return ap_pathfind_global(link, node, commit, max_distance);
}

int
ap_pathfind_sprite(size_t sprite_idx)
{
    struct xy link = ap_link_xy();
    struct ap_screen * screen = map_screens[XYMAPSCREEN(link)];
    assert(screen != NULL);
    assert_bp(XYIN(ap_sprites[sprite_idx].tl, screen->tl, screen->br));
    return ap_pathfind_local(screen, link, ap_sprites[sprite_idx].tl, ap_sprites[sprite_idx].br, true);
}

static void
array_reverse(void *array_base, size_t item_size, size_t array_len) {
    uint8_t buf[item_size];
    uint8_t (*array)[item_size] = array_base;
    for (size_t i = 0; i < array_len / 2; i++) {
        size_t j = array_len - 1 - i;
        assert(j > i);
        memcpy(buf, array[i], item_size);
        memcpy(array[i], array[j], item_size);
        memcpy(array[j], buf, item_size);
    }
}
static void
array_reverse_test() {
    char buf[32] = "123 456 789 abc def ";
    array_reverse(buf, 4, 5);
    assert_bp(strcmp(buf, "def abc 789 456 123 ") == 0);
    array_reverse(buf, 4, 4);
    assert_bp(strcmp(buf, "456 789 abc def 123 ") == 0);
    array_reverse(buf, 1, 3);
    assert_bp(strcmp(buf, "654 789 abc def 123 ") == 0);
}

int
ap_set_script(const struct ap_script * script) {
    LOG("Setting script: %s", script->name);
    assert(script->type == SCRIPT_SEQUENCE);
    assert(script->sequence != NULL);
    struct xy xy = script->start_tl;
    size_t i = 0;
    uint16_t dir_mask = SNES_MASK(UP) | SNES_MASK(DOWN) | SNES_MASK(LEFT) | SNES_MASK(RIGHT);
    ap_targets[i++] = (struct ap_target) { .tl = xy, .joypad = 0, .joypad_mask = 0 };
    for (const char *s = script->sequence; *s != '\0'; s++) {
        switch (*s) {
        case '<':
            xy.x -= 16;
            ap_targets[i++] = (struct ap_target) { .tl = xy, .joypad = 0, .joypad_mask = 0 };
            break;
        case '>':
            xy.x += 16;
            ap_targets[i++] = (struct ap_target) { .tl = xy, .joypad = 0, .joypad_mask = 0 };
            break;
        case '^':
            xy.y -= 16;
            ap_targets[i++] = (struct ap_target) { .tl = xy, .joypad = 0, .joypad_mask = 0 };
            break;
        case 'v':
            xy.y += 16;
            ap_targets[i++] = (struct ap_target) { .tl = xy, .joypad = 0, .joypad_mask = 0 };
            break;
        case 'A':
            ap_targets[i++] = (struct ap_target) { .tl = xy, .joypad_mask = SNES_MASK(A), .joypad = 0, };
            ap_targets[i++] = (struct ap_target) { .tl = xy, .joypad_mask = SNES_MASK(A), .joypad = SNES_MASK(A), };
            ap_targets[i++] = (struct ap_target) { .tl = xy, .joypad_mask = SNES_MASK(A), .joypad = 0, };
            break;
        case 'Y':
            ap_targets[i++] = (struct ap_target) { .tl = xy, .joypad_mask = SNES_MASK(Y), .joypad = 0, };
            ap_targets[i++] = (struct ap_target) { .tl = xy, .joypad_mask = SNES_MASK(Y), .joypad = SNES_MASK(Y), };
            ap_targets[i++] = (struct ap_target) { .tl = xy, .joypad_mask = SNES_MASK(Y), .joypad = 0, };
            break;
        case 'U':
            ap_targets[i++] = (struct ap_target) { .tl = xy, .joypad_mask = dir_mask, .joypad = 0, };
            ap_targets[i++] = (struct ap_target) { .tl = xy, .joypad_mask = dir_mask, .joypad = SNES_MASK(UP), };
            ap_targets[i++] = (struct ap_target) { .tl = xy, .joypad_mask = dir_mask, .joypad = 0, };
            break;
        default:
            LOG("Unhandled character in sequence: '%c'", *s);
            assert_bp(false);
            break;
        }
    }
    if (ap_targets[i].joypad != 0) {
        ap_targets[i+1] = ap_targets[i];
        ap_targets[i+1].joypad = 0;
        i++;
    }
    array_reverse(ap_targets, sizeof(*ap_targets), i);
    int timeout = ap_set_targets(i);
    ap_target_dst_tl = xy;
    ap_target_dst_br = XYOP1(xy, + 15);
    ap_target_screen = map_screens[XYMAPSCREEN(xy)];
    ap_target_scripted = true;

    LOG("Script timeout: %d", timeout);
    return timeout;
}

static void
ap_screen_add_raw_node(struct ap_screen * screen, struct ap_node * new_node)
{
    // Attach to screen
    new_node->screen = screen;
    LL_INIT(new_node);
    LL_PUSH(screen->node_list, new_node);
    LOG("attached node %s", new_node->name);

    // XXX This needs to be refactored to pair switches w/ doors
    // XXX Moved to ap_node_islocked
    /*
    if (new_node->type == NODE_SWITCH) {
        // Assign all doors to this switch
        for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
            if (node->lock_node == NULL && !XYEQ(node->locked_xy, XY(0, 0))) {
                LOGB("Assigning switch %s to unlock door %s", new_node->name, node->name);
                node->lock_node = new_node;
                break;
            }
        }
    }
    if (!XYEQ(new_node->locked_xy, XY(0, 0))) {
        // Assign the first switch we find to this door
        for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
            if (node->type == NODE_SWITCH) {
                LOGB("Assigning switch %s to unlock door %s", node->name, new_node->name);
                new_node->lock_node = node;
                break;
            }
        }
        LOGB("Unlock door %s = %p", new_node->name, new_node->lock_node);
    }
    */

    // Add goals
    switch (new_node->type) {
    case NODE_TRANSITION:
        if (add_explore_goals_global || (screen->info != NULL && screen->info->add_explore_goals)) {
            if (new_node->adjacent_node == NULL && new_node->adjacent_direction != 0)
                new_node->goal = ap_goal_add(GOAL_EXPLORE, new_node);
        }
        break;
    case NODE_ITEM:
        new_node->goal = ap_goal_add(GOAL_PICKUP, new_node);
        break;
    case NODE_CHEST:
        new_node->goal = ap_goal_add(GOAL_CHEST, new_node);
        break;
    case NODE_SWITCH:
        //ap_goal_add(GOAL_EXPLORE, new_node);
        break;
    case NODE_SPRITE:
        if (ap_sprite_attrs_for_type(new_node->sprite_type, new_node->sprite_subtype) & SPRITE_ATTR_TALK) {
            new_node->goal = ap_goal_add(GOAL_NPC, new_node);
        } else if (ap_sprite_attrs_for_type(new_node->sprite_type, new_node->sprite_subtype) & SPRITE_ATTR_ITEM)  {
            ap_goal_add(GOAL_PICKUP, new_node);
        }
        break;
    case NODE_SCRIPT:
        new_node->goal = ap_goal_add(GOAL_SCRIPT, new_node);
        break;
    case NODE_NONE:
    default:
        assert_bp(false);
        ;
    }

    // Update Graph
    /*
    if (new_node->goal != NULL) {
        ap_graph_add_prereq(&new_node->goal->graph, &screen->graph);

        for (struct ap_node * node2 = screen->node_list->next; node2 != screen->node_list; node2 = node2->next) {
            if (node2 == new_node) continue;
            if (new_node->goal == NULL || node2->goal == NULL) continue;
            if (new_node->type != NODE_TRANSITION && node2->type != NODE_TRANSITION) continue;
            int dist1 = ap_pathfind_local(screen, XYMID(new_node->tl, new_node->br), node2->tl, node2->br, false);
            int dist2 = ap_pathfind_local(screen, XYMID(node2->tl, node2->br), new_node->tl, new_node->br, false);
            if (dist1 >= 0 && dist2 >= 0) {
                if (new_node->type == NODE_TRANSITION && node2->type == NODE_TRANSITION) {
                    // XXX only the first new equiv is actually added; transitivity is assumed
                    ap_graph_add_equiv(&node2->goal->graph, &new_node->goal->graph);
                } else if (new_node->type == NODE_TRANSITION) {
                    ap_graph_add_prereq(&node2->goal->graph, &new_node->goal->graph);
                } else {
                    ap_graph_add_prereq(&new_node->goal->graph, &node2->goal->graph);
                }
            }
        }
    }
    */

    /*
    if (strcmp(new_node->name, "0x198A D 4") == 0) {
        new_node->_debug_blocked = false;
    } else if (strcmp(new_node->name, "0x1483 L 3") == 0) {
        new_node->_debug_blocked = true;
    } else if (strcmp(new_node->name, "0x148A L 4 l") == 0) {
        new_node->_debug_blocked = true;
    //} else if (new_node->screen->id == 0x0e50 && strcmp(new_node->name, "stairs U 0x5e") == 0) {
    //    new_node->_debug_blocked = true;
    } else if (new_node->screen->id == 0x0c0a && strncmp(new_node->name, "door", 4) == 0) {
        new_node->_debug_blocked = true;
    } else if (new_node->screen->id == 0x0402 && strncmp(new_node->name, "door", 4) == 0) {
        new_node->_debug_blocked = true;
    }
    */

}

static struct ap_node *
ap_screen_commit_node(struct ap_screen * screen, struct ap_node ** new_node_p)
{
    struct ap_node * new_node = *new_node_p;
    NONNULL(new_node);

    if (new_node->tl.x == 0 && new_node->tl.y == 0)
        return NULL;
    assert(new_node->type != NODE_NONE);
    assert(XYUNDER(new_node->tl, new_node->br));

    // Check if the node already exists
    for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
        if (node->type != new_node->type)
            continue;
        if (node->adjacent_direction != new_node->adjacent_direction)
            continue;
        if (node->type != NODE_SPRITE) {
            if (XYEQ(node->tl, new_node->tl) && XYEQ(node->br, new_node->br)) {
                //assert_bp(node->tile_attr == new_node->tile_attr);
            } else if (XYIN(new_node->tl, node->tl, node->br) && XYIN(new_node->br, node->tl, node->br)) {
                //assert(node->adjacent_screen == new_node->adjacent_screen);
                // XXX This has a lot of false positives
                //assert_bp(node->tile_attr == new_node->tile_attr);

                //LOG("duplicate node %s == %s", node->name, new_node->name);

                // Merge in new information???
                LOGB("node tl/br would shrink: before: " PRIBBV " after: " PRIBBV, PRIBBVF(*node), PRIBBVF(*new_node));
                new_node->tl = node->tl;
                new_node->br = node->br;
            } else if (node->type == NODE_TRANSITION && XYIN(node->tl, new_node->tl, new_node->br) && XYIN(node->br, new_node->tl, new_node->br)) {
                //assert_bp(node->tile_attr == new_node->tile_attr);
                LOGB("node tl/br grew: before: " PRIBBV " after: " PRIBBV, PRIBBVF(*node), PRIBBVF(*new_node));
            } else {
                continue;
            }
        }

        node->tl = new_node->tl;
        node->br = new_node->br;

        memset(new_node, 0, sizeof *new_node);
        return node;
    }

    // Merge paired stairs from upper to lower levels
    if (new_node->type == NODE_TRANSITION && (ap_tile_attrs[new_node->tile_attr] & TILE_ATTR_STRS) && new_node->adjacent_node == NULL && XYINDOORS(new_node->tl)) {
        struct xy alt_xy = XYOP2(XYFLIPBG(new_node->tl), + 48 *, dir_dxy[new_node->adjacent_direction]);
        struct ap_screen * alt_screen = map_screens[XYMAPSCREEN(alt_xy)];
        if (alt_screen != NULL) {
            for (struct ap_node * node = alt_screen->node_list->next; node != alt_screen->node_list; node = node->next) {
                if (node->type != NODE_TRANSITION)
                    continue;
                if (node->adjacent_node != NULL)
                    continue;
                if (node->adjacent_direction != dir_opp[new_node->adjacent_direction])
                    continue;
                if (node->tile_attr != new_node->tile_attr)
                    continue;
                if (!XYIN(alt_xy, node->tl, node->br))
                    continue;
                //LOG("Paring nodes: %s <-> %s", node->name, new_node->name);
                node->adjacent_node = new_node;
                new_node->adjacent_node = node;
                break;
            }
        }
    }

    ap_screen_add_raw_node(screen, new_node);

    *new_node_p = NONNULL(calloc(1, sizeof **new_node_p));
    return new_node;
}

static void
ap_update_map_screen_nodes()
{
    // Update the nodes that already exist on the screen
    struct xy tl, br;
    ap_map_bounds(&tl, &br);
    size_t index = XYMAPSCREEN(tl);
    struct ap_screen * screen = NONNULL(map_screens[index]);

    for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
        switch (node->type) {
        case NODE_SWITCH:;
            uint16_t mask = 0x0;
            switch(node->tile_attr) {
                case 0x23: mask = 0x8000; break;
                case 0x24: mask = 0x4000; break;
                case 0x25: mask = 0x2000; break; // XXX Wild guess
                case 0x26: mask = 0x1000; break; // XXX Wild guess
                default: assert(false);
            }
            if (*ap_ram.room_state & mask) {
                // what to update?
            }
            break;
        case NODE_SPRITE:
            break;
        case NODE_CHEST:
        case NODE_ITEM:
        case NODE_TRANSITION:
        case NODE_NONE:
        default:;
        }
    }
}

static void ap_map_add_nodes_to_screen(struct ap_screen * screen);
static void ap_map_add_scripts_to_screen(struct ap_screen * screen);

static int ap_node_distance_cmp(const void *a, const void *b) {
    return memcmp(a, b, sizeof(struct ap_node_distance));
}

static void ap_map_screen_update_distances(struct ap_screen * screen) {
    size_t n_transition_nodes = 0;
    size_t n_other_nodes = 0;
    for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
        if (node->type == NODE_TRANSITION) {
            n_transition_nodes++;
        } else {
            n_other_nodes++;
        }
    }

    size_t req_capcacity = n_transition_nodes * (n_transition_nodes + n_other_nodes);
    if (req_capcacity > screen->distances_capacity) {
        screen->distances = NONNULL(realloc(screen->distances, req_capcacity * sizeof(*screen->distances)));
        screen->distances_capacity = req_capcacity;
    }
    memset(screen->distances, 0, req_capcacity * sizeof(*screen->distances));

    size_t d = 0;
    for (struct ap_node * src = screen->node_list->next; src != screen->node_list; src = src->next) {
        if (src->type != NODE_TRANSITION) {
            continue;
        }
        for (struct ap_node * dst = screen->node_list->next; dst != screen->node_list; dst = dst->next) {
            int distance = ap_pathfind_local(screen, XYMID(src->tl, src->br), dst->tl, dst->br, false);
            if (distance < 0) {
                continue;
            }
            screen->distances[d++] = (struct ap_node_distance) {
                .src = src,
                .dst = dst,
                .distance = (uint64_t) distance,
            };
        }
    }
    assert(d <= req_capcacity);
    screen->distances_length = d;
    qsort(screen->distances, d, sizeof(*screen->distances), &ap_node_distance_cmp);
}

struct ap_screen *
ap_update_map_screen(bool force)
{
    struct xy tl, br;
    ap_map_bounds(&tl, &br);
    size_t index = XYMAPSCREEN(tl);
    if (index == last_map_index && !force) {
        ap_update_map_screen_nodes();
        return NONNULL(map_screens[index]);
    }
    last_map_index = index;

    struct ap_screen * screen = map_screens[index];
    if (screen == NULL) {
        struct xy cells[8] = {
            XY(0x000, 0x000), XY(0x100, 0x000),
            XY(0x000, 0x100), XY(0x100, 0x100),
            XY(0x200, 0x000), XY(0x300, 0x000),
            XY(0x200, 0x100), XY(0x300, 0x100),
        };
        bool indoors = XYINDOORS(tl);
        for (size_t i = 0; i < (indoors ? 8 : 1); i++) {
            struct xy cell_tl, cell_br;
            if (indoors) {
                cell_tl = XYOP2(XYOP2(tl, &~, cells[7]), |, cells[i]);
                ap_map_room_bounds(cell_tl, &cell_tl, &cell_br);
            } else {
                cell_tl = tl;
                cell_br = br;
            }
            if (map_screens[XYMAPSCREEN(cell_tl)] != NULL) continue;

            screen = calloc(1, sizeof *screen);
            screen->tl = cell_tl;
            screen->br = cell_br;
            screen->id = (screen->tl.x >> 8) | (screen->tl.y & 0xFF00);
            const char * suffix = "";
            if (XYINDOORS(screen->tl)) {
                if (XYONUPPER(screen->tl)) {
                    screen->id &= ~0x2;
                    suffix = " ^";
                } else {
                    suffix = " v";
                }
            }
            for (const struct ap_screen_info * info = ap_screen_infos; info->id != (uint16_t) -1; info++) {
                if (info->id == screen->id) {
                    screen->info = info;
                    break;
                }
            }
            //ap_graph_init(&screen->graph, screen->name);
            LL_INIT(screen->node_list);
            if (screen->info) {
                snprintf(screen->name, sizeof screen->name, "%s%s " PRIXY " x " PRIXY, screen->info->name, suffix, PRIXYF(cell_tl), PRIXYF(cell_br));
            } else {
                snprintf(screen->name, sizeof screen->name, "%#06x%s " PRIXYV " x " PRIXYV, screen->id, suffix, PRIXYVF(cell_tl), PRIXYVF(cell_br));

            }

            struct xy xy;
            for (xy.y = cell_tl.y; xy.y < cell_br.y; xy.y += 0x100) {
                for (xy.x = cell_tl.x; xy.x < cell_br.x; xy.x += 0x100) {
                    map_screens[XYMAPSCREEN(xy)] = screen;
                    map_screen_mask_x[xy.x / 0x100] = true;
                    map_screen_mask_y[xy.y / 0x100] = true;
                }
            }

            ap_screen_refresh_cache(screen);
            ap_map_add_nodes_to_screen(screen);
            ap_map_add_scripts_to_screen(screen);
            ap_map_screen_update_distances(screen);
        }
        screen = map_screens[index];
    } else {
        ap_screen_refresh_cache(screen);
        ap_map_add_nodes_to_screen(screen);
    }

    ap_map_screen_update_distances(screen);

    //ap_graph_mark_done(&screen->graph);
    ap_sprites_print();
    ap_print_map_screen(screen);
    // TODO: Skip this if link is not yet on the screen
    printf("Nodes: [(un)Reachable? (un)Adjacent?] (tl x br) type \"name\"\n");

    int i = 0; 
    for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
        bool reachable = ap_pathfind_local(screen, ap_link_xy(), node->tl, node->br, false) >= 0;
        node->_reachable = reachable;
        printf("   %d. %p [%c%c] (" PRIXY " x " PRIXY ") %s \"%s\"\n",
                i++, node, "uR"[reachable], "uA"[node->adjacent_node != NULL], PRIXYF(node->tl), PRIXYF(node->br), ap_node_type_names[node->type], node->name);

    }

    return screen;
}

static void ap_map_add_scripts_to_screen(struct ap_screen * screen) {
    for (size_t i = 0; i < ARRAYLEN(ap_scripts); i++) {
        const struct ap_script * script = &ap_scripts[i];
        if (!XYIN(script->start_tl, screen->tl, screen->br)) {
            continue;
        }

        struct ap_node * new_node = NONNULL(calloc(1, sizeof *new_node));
        new_node->type = NODE_SCRIPT;
        new_node->tl = script->start_tl;
        new_node->br = XYOP1(script->start_tl, +15);
        new_node->script = script;
        snprintf(new_node->name, sizeof new_node->name, "Script: %s", script->name);

        ap_screen_add_raw_node(screen, new_node);
    }
}

static void
ap_map_add_sprite_nodes_to_screen(struct ap_screen * screen) {
    static struct ap_node * new_node = NULL;
    if (new_node == NULL)
        new_node = NONNULL(calloc(1, sizeof *new_node));

    for (size_t i = 0; i < 16; i++) {
        if (ap_sprites[i].type == 0)
            continue;
        if (ap_sprites[i].attrs & SPRITE_ATTR_NODE) {
            new_node->tl = ap_sprites[i].tl;
            if (!XYIN(new_node->tl, screen->tl, screen->br)) {
                LOG("Sprite %zu out of screen", i);
            } else {
                new_node->type = NODE_SPRITE;
                new_node->br = ap_sprites[i].br;
                new_node->sprite_type = ap_sprites[i].type;
                new_node->sprite_subtype = ap_sprites[i].subtype;
                /*
                if (ap_sprites[i].attrs & SPRITE_ATTR_TALK) {
                    new_node->tl = XYOP2(ap_sprites[i].hitbox_tl, +, XY(0, 16));
                    new_node->br = XYOP2(ap_sprites[i].hitbox_tl, +, XY(15, 31));
                }
                */
                snprintf(new_node->name, sizeof new_node->name, "sprite %#x.%#x %s", new_node->sprite_type, new_node->sprite_subtype, ap_sprite_attr_name(ap_sprites[i].attrs));
                ap_screen_commit_node(screen, &new_node);
            }
        }
    }
}

static void
ap_map_add_nodes_to_screen(struct ap_screen * screen) {
    struct xy tl = screen->tl;
    struct xy br = screen->br;
    size_t index = XYMAPSCREEN(tl);

    int new_node_count = 0;
    static struct ap_node * new_node = NULL;
    if (new_node == NULL)
        new_node = NONNULL(calloc(1, sizeof *new_node));

    //uint16_t walk_mask = TILE_ATTR_WALK | TILE_ATTR_SWIM;
    uint16_t walk_mask = TILE_ATTR_WALK | TILE_ATTR_DOOR;
    const uint8_t dirs[4] = {
        // Top, Botton, Left, Right edges
        DIR_U, DIR_D, DIR_L, DIR_R,
    };
    const struct xy xy_init[4] = {
        tl, XY(tl.x, br.y & ~7), tl, XY(br.x & ~7, tl.y),
    };
    const struct xy xy_step[4] = {
        dir_dxy[DIR_R], dir_dxy[DIR_R], dir_dxy[DIR_D], dir_dxy[DIR_D],
    };
    uint16_t masks[4] = {
        walk_mask, walk_mask, walk_mask, walk_mask,
    };
    uint8_t f = 2;

    for (uint8_t k = 0; k < 4; k++) {
        uint8_t i = dirs[k];
        struct xy new_start = XY(0, 0);
        struct xy new_end = XY(0, 0);
        size_t size = 0;

        for (struct xy xy = xy_init[k]; XYIN(xy, tl, br); xy = XYOP2(xy, +, XYOP1(xy_step[k], * 8))) {
            for (; XYIN(xy, tl, br); xy = XYOP2(xy, +, XYOP1(xy_step[k], * 8))) {
                bool can_walk = true;
                //if (XYINDOORS(xy) && (ap_map_attr(xy) == 0x00 || ap_map_attr(xy) == 0x1c)) {
                if (XYINDOORS(xy) && (ap_map_attr(xy) == 0x1c)) {
                    // NOTE: sometimes 0's are actually walkable
                    // inside, border 0 or 1c is just empty
                    // you could walk there, but you definitely can't get there
                    can_walk = false;
                }
                for (int8_t j = 0; j < f + 2; j++) {
                    struct xy lxy = XYOP2(xy, -, XYOP1(dir_dxy[i], * 8 * j));
                    if (!(ap_tile_attrs[ap_map_attr(lxy)] & masks[k])) {
                        can_walk = false;
                        break;
                    }
                }
                if (can_walk) {
                    new_end = XYOP2(xy, -, XYOP1(dir_dxy[i], * 8 * (f + 1)));
                    for (int8_t j = f + 1; j < f + 8; j++) {
                        struct xy lxy = XYOP2(new_end, -, XYOP1(dir_dxy[i], * 8));
                        uint8_t attr = ap_map_attr(lxy);
                        // 0x80 through 0x9F are walkways between rooms
                        // 0xF0 through 0xF8 are locked doors?
                        if (attr < 0x80)
                            break;
                        else if (attr > 0x9F && attr < 0xF0)
                            break;
                        else if (attr > 0xF8)
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
                    break;
                }
            }
            new_node->tl = XYFN2(MIN, new_start, new_end);
            new_node->br = XYFN2(MAX, new_start, new_end);
            new_node->br = XYOP1(new_node->br, + 7);
            new_node->type = NODE_TRANSITION;
            new_node->adjacent_direction = i;
            new_node->tile_attr = ap_map_attr(ap_box_edge(new_node->tl, new_node->br, i));
            new_node->locked_xy = XY(0, 0);
            if (new_node->tile_attr == 0x82 || new_node->tile_attr == 0x83) { // locked door
                new_node->locked_xy = ap_box_edge(new_node->tl, new_node->br, dir_opp[i]);
                new_node->locked_xy = XYOP1(new_node->locked_xy, &~7);
                uint8_t attr = ap_map_attr(new_node->locked_xy);
            }

            //new_node->adjacent_screen = &map_screens[XYMAPSCREEN(XYOP2(new_node->tl, +, XYOP1(dir_dxy[i], * 0x100)))];
            snprintf(new_node->name, sizeof new_node->name, "0x%02zX %s %d%s", index, dir_names[i], ++new_node_count, XYEQ(new_node->locked_xy, XY(0, 0)) ? "" : " l");
            ap_screen_commit_node(screen, &new_node);
            new_start = new_end = XY(0, 0);
            size = 0;
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
    if (XYONUPPER(tl)) {
        // Inside on upper level, look for non-diagonal ledges
        // and add them as nodes
        for (struct xy xy = tl; xy.y < br.y; xy.y += 0x8) {
            for (xy.x = tl.x; xy.x < br.x; xy.x += 0x8) {
                uint8_t attr = ap_map_attr(xy);
                int d = attr - 0x27;
                if (d <= 0 || d >= 5)
                    continue;
                // In dungeons 0x28 is used for both up & down; 0x2A for both left & right
                // Look for the 0x1C "hole" to fall into
                if (ap_map_attr(XYOP2(xy, + 8*, dir_dxy[d])) != 0x1C)
                    d++;
                if (ap_map_attr(XYOP2(xy, + 8*, dir_dxy[d])) != 0x1C)
                    continue;
                static const int perp_dirs[5] = {0, DIR_R, DIR_R, DIR_D, DIR_D};
                uint8_t adj_attr = ap_map_attr(XYOP2(xy, - 8*, dir_dxy[perp_dirs[d]]));
                if (adj_attr == attr)
                    continue; // already made a node for this
                struct xy l_tl = xy;
                struct xy l_br = xy;
                bool valid_ledge = true;
                int ledge_size = 0;
                bool found_start = false;
                for (struct xy t = xy; XYUNDER(t, br); t = XYOP2(t, + 8*, dir_dxy[perp_dirs[d]])) {
                    uint8_t t_attr = ap_map_attr(t);
                    if (ap_tile_attrs[t_attr] & TILE_ATTR_STRS) {
                        // If there are stairs, omit the ledge (because you can just take the stairs)
                        valid_ledge = false;
                    } else if (t_attr != attr) {
                        break;
                    } else if (ap_tile_attrs[ap_map_attr(XYOP2(t, - 8*, dir_dxy[d]))] & TILE_ATTR_WALK) {
                        if (!found_start) {
                            found_start = true;
                            l_tl = t;
                        }
                        ledge_size++;
                    } else if (found_start) {
                        break;
                    }
                    l_br = t;
                }
                if (found_start && valid_ledge && ledge_size >= 2) {
                    struct xy ledge_tl = l_tl;
                    struct xy ledge_br = XYOP1(l_br, + 7);
                    struct xy offset = XYOP1(dir_dxy[d], * 8);
                    if (d == DIR_D || d == DIR_R) {
                        ledge_tl = XYOP2(ledge_tl, -, XYOP1(offset, * 2));
                    } else {
                        ledge_br = XYOP2(ledge_br, -, XYOP1(offset, * 2));
                    }
                    // landing target
                    new_node->type = NODE_TRANSITION;
                    new_node->adjacent_direction = 0;
                    new_node->tl = XYOP2(ledge_tl, +, XYOP1(offset, * 3));
                    new_node->br = XYOP2(ledge_br, +, XYOP1(offset, * 3));
                    new_node->tl.x ^= 0x200;
                    new_node->br.x ^= 0x200;
                    new_node->tile_attr = 0x1C; // a lie
                    snprintf(new_node->name, sizeof new_node->name, "ledge %s landing", dir_names[d]);
                    struct ap_screen * bottom_screen = NONNULL(map_screens[XYMAPSCREEN(new_node->tl)]);
                    struct ap_node * landing = ap_screen_commit_node(bottom_screen, &new_node);
                    new_node_count++;

                    new_node->tile_attr = attr;
                    new_node->type = NODE_TRANSITION;
                    new_node->adjacent_direction = d;
                    new_node->tl = ledge_tl;
                    new_node->br = ledge_br;
                    new_node->adjacent_node = landing;
                    snprintf(new_node->name, sizeof new_node->name, "ledge %sx%d", dir_names[d], ledge_size);
                    ap_screen_commit_node(screen, &new_node);
                    new_node_count++;
                }

                xy.x = l_br.x;
            }
        }
    }
    for (struct xy xy = tl; xy.y < br.y; xy.y += 0x10) {
        for (xy.x = tl.x; xy.x < br.x; xy.x += 0x10) {
            uint8_t attr = ap_map_attr(xy);
            if (!(ap_tile_attrs[attr] & TILE_ATTR_NODE))
                continue;
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
            new_node->tile_attr = attr;
            const char * attr_name = ap_tile_attr_name(attr);
            if (ap_tile_attrs[attr] & TILE_ATTR_STRS) {
                if (attr == 0x5e || attr == 0x5f) {
                    //new_node->tl.y += 24;
                    //new_node->br.y += 24;
                }
                new_node->type = NODE_TRANSITION;
                if ((attr & 0xF0) == 0x10 || attr == 0x5E || attr == 0x3F) {
                    new_node->adjacent_direction = DIR_U;
                } else {
                    new_node->adjacent_direction = DIR_D;
                }
                if (attr == 0x3E || attr == 0x3F) {
                    if (xy.x & 0x200) { // on upper level
                        new_node->adjacent_direction = dir_opp[new_node->adjacent_direction];
                    }
                    struct xy offset = XYOP1(dir_dxy[new_node->adjacent_direction], * -24);
                    new_node->tl = XYOP2(new_node->tl, +, offset);
                    new_node->br = XYOP2(new_node->br, +, offset);
                } else if ((ap_map_attr(XYOP2(new_node->tl, -, XY(0, 8))) & 0xF0) == 0x30) {
                    // Check for 0x30 family which does a screen transition
                    new_node->adjacent_direction = DIR_U;
                } else if ((ap_map_attr(XYOP2(new_node->tl, +, XY(0, 16))) & 0xF0) == 0x30) {
                    new_node->adjacent_direction = DIR_D;
                } else {
                    if (xy.x & 0x200) { // on upper level
                        new_node->adjacent_direction = dir_opp[new_node->adjacent_direction];
                    }
                    struct xy offset = XYOP1(dir_dxy[new_node->adjacent_direction], * -24);
                    new_node->tl = XYOP2(new_node->tl, +, offset);
                    new_node->br = XYOP2(new_node->br, +, offset);
                }
                snprintf(new_node->name, sizeof new_node->name, "stairs %c 0x%02x", new_node->adjacent_direction == DIR_U ? 'U' : 'D', attr);
            } else if (ap_tile_attrs[attr] & TILE_ATTR_DOOR) {
                new_node->type = NODE_TRANSITION;
                struct xy screen_center = XYMID(screen->tl, screen->br);
                struct xy node_center = XYMID(new_node->tl, new_node->br);
                int dx = (int) node_center.x - (int) screen_center.x;
                int dy = (int) node_center.y - (int) screen_center.y;
                if (ABS(dx) < ABS(dy)) {
                    if (dy > 0) {
                        new_node->adjacent_direction = DIR_D;
                    } else {
                        new_node->adjacent_direction = DIR_U;
                    }
                } else {
                    if (dx > 0) {
                        new_node->adjacent_direction = DIR_R;
                    } else {
                        new_node->adjacent_direction = DIR_L;
                    }
                }

                snprintf(new_node->name, sizeof new_node->name, "door %s 0x%02x %s", dir_names[new_node->adjacent_direction], attr, attr_name);
            } else if (ap_tile_attrs[attr] & TILE_ATTR_CHST) {
                new_node->type = NODE_CHEST;
                // Chests are opened from the bottom
                new_node->tl.y += 16;
                new_node->br.y += 16;
                snprintf(new_node->name, sizeof new_node->name, "chest 0x%02x %s", attr, attr_name);
            } else if (ap_tile_attrs[attr] & TILE_ATTR_SWCH) {
                new_node->type = NODE_SWITCH;
                new_node->adjacent_direction = 0;
                snprintf(new_node->name, sizeof new_node->name, "switch 0x%02x %s", attr, attr_name);
            } else if (ap_tile_attrs[attr] & TILE_ATTR_LFT0) {
                size_t i;
                for (i = 0; i < ARRAYLEN(ap_pushblocks); i++) {
                    if (XYEQ(ap_pushblocks[i].tl, new_node->tl)) {
                        break;
                    }
                }
                if (i != ARRAYLEN(ap_pushblocks)) {
                    continue;
                }
                new_node->type = NODE_ITEM;
                snprintf(new_node->name, sizeof new_node->name, "pot 0x%02x %s", attr, attr_name);
            } else {
                new_node->type = NODE_ITEM;
                snprintf(new_node->name, sizeof new_node->name, "unknown item!? 0x%02x %s", attr, attr_name);
            }
            ap_screen_commit_node(screen, &new_node);
            new_node_count++;
        }
    }

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
        if (dir_opp[node->adjacent_direction] != src_node->adjacent_direction)
            continue;
        if (XYL1BOXDIST(dst_xy, node->tl, node->br))
            continue;
        dst_node = node;
        break;
    }
    if (dst_node == NULL) {
        LOGB("Warning! dst_node is NULL so making own node");
        static struct ap_node * new_node = NULL;
        if (new_node == NULL)
            new_node = NONNULL(calloc(1, sizeof *new_node));

        // TODO: Should be based on the size of src_node?
        new_node->tl = XYOP1(dst_xy, & ~7);
        new_node->br = XYOP1(new_node->tl, + 15);
        new_node->type = NODE_TRANSITION;
        if (ap_tile_attrs[src_node->tile_attr] & TILE_ATTR_LDGE)  {
            new_node->adjacent_direction = 0;
        } else if (src_node->tile_attr == 0x5e || src_node->tile_attr == 0x5f) {
            new_node->adjacent_direction = DIR_U;
            new_node->tile_attr = src_node->tile_attr ^ 0x1; // XXX
        } else {
            new_node->adjacent_direction = dir_opp[src_node->adjacent_direction];
        }
        snprintf(new_node->name, sizeof new_node->name, "0x%02X %s cross", XYMAPSCREEN(dst_xy), dir_names[new_node->adjacent_direction]);

        dst_node = ap_screen_commit_node(dst_screen, &new_node);
    }
    assert(dst_node != NULL);

    LOGB("Attaching nodes from transition: %s <-> %s", dst_node->name, src_node->name);
    src_node->adjacent_node = dst_node;
    dst_node->adjacent_node = src_node;

    return 0;
}

bool
ap_node_islocked(struct ap_node * node) {
    if (node->type != NODE_TRANSITION)
        return false;
    if (ap_update_map_screen(false) != node->screen)
        return false;
    if (!*ap_ram.room_trap_doors) return false;
    if (node->screen->info != NULL) {
        if (node->screen->info->key_doors && *ap_ram.dungeon_current_keys > 0) {
            return false;
        }
    }

    uint8_t tile_attr_tl = ap_map_attr(node->tl);
    uint8_t tile_attr_br = ap_map_attr(node->br);
    if ((tile_attr_tl & 0xF0) == 0xF0 || (tile_attr_br & 0xF0) == 0xF0) {
        // Just-in-time assign switch unlock nodes
        if (node->lock_node == NULL) {
            for (struct ap_node * sw_node = node->screen->node_list->next; sw_node != node->screen->node_list; sw_node = sw_node->next) {
                if (sw_node->type == NODE_SWITCH) {
                    LOGB("Assigning switch %s to unlock door %s", sw_node->name, node->name);
                    node->lock_node = sw_node;
                    break;
                }
            }
        }

        return true;
    }
    return false;
}

void
ap_map_export(const char * filename) {
    FILE * f = fopen(filename, "w");
    if (f == NULL) return;
    for (size_t s = 0; s < 0x100 * 0x100; s++) {
        struct ap_screen *screen = map_screens[s];
        if (screen == NULL) continue;
        if (XYMAPSCREEN(screen->tl) != s) continue;
        fprintf(f, ">0x%04x 0x%04x,0x%04x 0x%04x,0x%04x # %s\n",
                screen->id, screen->tl.x, screen->tl.y, screen->br.x, screen->br.y, screen->name);
        for (const struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
            fprintf(f, "@%p 0x%04x,0x%04x 0x%04x,0x%04x",
                    node, node->tl.x, node->tl.y, node->br.x, node->br.y);
            fprintf(f, " %u %u %p %d %u %u %s\n",
                    node->type, node->adjacent_direction, node->adjacent_node, node->tile_attr, node->sprite_type, node->sprite_subtype, node->name);
        }
        for (size_t y = 0; y < 0x80; y++) {
            fprintf(f, "|");
            for (size_t x = 0; x < 0x80; x++) {
                fprintf(f, "%02x ", screen->attr_cache[y][x]);
            }
            fprintf(f, "\n");
        }
    }
    fclose(f);
}

void
ap_map_import(const char * filename) {
    array_reverse_test();
    FILE * f = fopen(filename, "r");
    if (f == NULL) return;
    struct pm * pm = pm_create();
    struct ap_screen *screen = NULL;
    size_t attr_row = 0;
    char *line = NULL;
    size_t line_len = 0;
    size_t line_number = 0;
    while (getline(&line, &line_len, f) != -1) {
        line_number++;
        if (line[0] == '#') {
            continue;
        } else if (line[0] == '>') {
            screen = NONNULL(calloc(1, sizeof *screen));
            int rc = sscanf(line, ">0x%hx 0x%hx,0x%hx 0x%hx,0x%hx",
                &screen->id, &screen->tl.x, &screen->tl.y, &screen->br.x, &screen->br.y);
            assert_bp(rc == 5);
            for (const struct ap_screen_info * info = ap_screen_infos; info->id != (uint16_t) -1; info++) {
                if (info->id == screen->id) {
                    screen->info = info;
                    break;
                }
            }
            //ap_graph_init(&screen->graph, screen->name);
            LL_INIT(screen->node_list);
            const char * suffix = "";
            if (XYINDOORS(screen->tl)) {
                if (XYONUPPER(screen->tl)) {
                    screen->id &= ~0x2;
                    suffix = " ^";
                } else {
                    suffix = " v";
                }
            }
            if (screen->info) {
                snprintf(screen->name, sizeof screen->name, "%s%s " PRIXY " x " PRIXY, screen->info->name, suffix, PRIXYF(screen->tl), PRIXYF(screen->br));
            } else {
                snprintf(screen->name, sizeof screen->name, "%#06x%s " PRIXYV " x " PRIXYV, screen->id, suffix, PRIXYVF(screen->tl), PRIXYVF(screen->br));

            }

            struct xy xy;
            for (xy.y = screen->tl.y; xy.y < screen->br.y; xy.y += 0x100) {
                for (xy.x = screen->tl.x; xy.x < screen->br.x; xy.x += 0x100) {
                    map_screens[XYMAPSCREEN(xy)] = screen;
                    map_screen_mask_x[xy.x / 0x100] = true;
                    map_screen_mask_y[xy.y / 0x100] = true;
                }
            }

            ap_map_add_scripts_to_screen(screen);

            attr_row = 0;
        } else if (line[0] == '@') {
            assert(screen != NULL);
            struct ap_node * node = NONNULL(calloc(1, sizeof *node));
            void * node_ptr = 0;
            void * adj_node_ptr = 0;
            int node_type = NODE_NONE;
            int rc = sscanf(line, "@%p 0x%hx,0x%hx 0x%hx,0x%hx %d %hhu %p %hhd %hhu %hu %[^\n]",
                &node_ptr, &node->tl.x, &node->tl.y, &node->br.x, &node->br.y,
                &node_type, &node->adjacent_direction, &adj_node_ptr, &node->tile_attr, &node->sprite_type, &node->sprite_subtype, node->name);
            assert_bp(rc == 12);
            node->type = node_type;
            if (node->type == NODE_SCRIPT) {
                free(node);
                continue;
            }
            assert(pm_set(pm, (uintptr_t) node_ptr, node) == 0);
            if (adj_node_ptr != 0) {
                assert(pm_get(pm, (uintptr_t) adj_node_ptr, (void **) &node->adjacent_node) == 0);
            }

            ap_screen_add_raw_node(screen, node);
        } else if (line[0] == '|') {
            assert(screen != NULL);
            assert(attr_row < 0x80);
            char * buf = &line[1];
            for (size_t i = 0; i < 0x80; i++) {
                char * attr = strsep(&buf, " \n");
                screen->attr_cache[attr_row][i] = strtoul(attr, NULL, 16);
            }
            attr_row++;
        } else {
            LOG("unexpected line #%zu: %s", line_number, line);
            assert(0);
        }
    }
    free(line);
    size_t unmatched_pm = pm_destroy(pm);
    LOG("unmapped: %zu", unmatched_pm);
    assert_bp(unmatched_pm == 0);

    for (struct xy xy = XY(0, 0); xy.y < 0x8000; xy.y += 0x100) {
        for (xy.x = 0; xy.x < 0xC000; xy.x += 0x100) {
            struct ap_screen * screen = map_screens[XYMAPSCREEN(xy)];
            if (screen == NULL || !XYEQ(screen->tl, xy)) continue;
            ap_map_screen_update_distances(screen);
        }
    }
}

void
ap_map_tick() {
    struct ap_screen * screen = ap_update_map_screen(false);
    if (ap_sprites_changed) {
        ap_map_add_sprite_nodes_to_screen(screen);
    }
}
