#include "ap_snes.h"
#include "ap_macro.h"
#include "ap_math.h"
#include "ap_map.h"
#include "ap_plan.h"

uint64_t ap_frame = 0;
struct ap_ram ap_ram;
struct ap_snes9x * ap_emu = NULL;
bool ap_manual_mode = false;

char ap_info_string[INFO_STRING_SIZE];
bool ap_debug;
struct ap_sprite ap_sprites[16];
bool ap_sprites_changed;
struct ap_pushblock ap_pushblocks[16];

const char * const ap_tile_attr_names[] = {
#define X(d) [CONCAT(TILE_ATTR_, d)] = STRINGIFY(d),
TILE_ATTR_LIST
#undef X
};

const char * ap_tile_attr_name(uint8_t idx) {
    uint16_t attr = ap_tile_attrs[idx];
    static char sbuf[1024];
    char * buf = sbuf;
#define X(d) if (attr & CONCAT(TILE_ATTR_, d)) { \
    if (buf != sbuf) { *buf++ = '|'; } \
    buf += sprintf(buf, STRINGIFY(d)); \
    }
TILE_ATTR_LIST
#undef X
    return sbuf;
}

const char * const ap_sprite_attr_names[] = {
#define X(d) [CONCAT(SPRITE_ATTR_, d)] = STRINGIFY(d),
SPRITE_ATTR_LIST
#undef X
};

const char * ap_sprite_attr_name(uint16_t attr) {
    static char sbuf[1024];
    char * buf = sbuf;
#define X(d) if (attr & CONCAT(SPRITE_ATTR_, d)) { \
    if (buf != sbuf) { *buf++ = '|'; } \
    buf += sprintf(buf, STRINGIFY(d)); \
    }
SPRITE_ATTR_LIST
#undef X
    return sbuf;
}


void
ap_init(struct ap_snes9x * _emu)
{
    ap_emu = _emu;

#define X(name, type, offset) ap_ram.name = (type *) ap_emu->base(offset);
AP_RAM_LIST
#undef X

    printf("----- initialized ------ \n");
    ap_emu->load("home");
    //ap_emu->load("well");
    //ap_emu->load("castle");
    //ap_emu->load("estpal");
    //ap_emu->load("epsta");
    //ap_emu->load("prearmos");
    //ap_emu->load("cave_front");
    //ap_emu->load("cave");
    //ap_emu->load("hc_stairs");
    //ap_emu->load("rock");
    //ap_emu->load("stair_test");
    //ap_emu->load("near_hole");
    //ap_emu->load("mmcave");
    //ap_emu->load("basement");
    //ap_emu->load("dam_puzzle");
    //ap_emu->load("blinds_house");
    //ap_emu->load("atchest");
    //ap_emu->load("pyramid");
    ap_emu->load("hpegs");

    ap_plan_init();
}

void
ap_ram_print()
{
    LOG("All RAM:");
    size_t i = 0;
#define X(name, type, offset) \
    printf("%24s: %#-10x", STRINGIFY(name), *ap_ram.name); \
    if (i++ % 6 == 5) { printf("\n"); }
AP_RAM_LIST
#undef X
}

uint16_t
ap_sprite_attrs_for_type(uint8_t type, uint16_t subtype, uint16_t dungeon_room)
{
    uint16_t attrs = ap_sprite_attrs[type];
    if (attrs & SPRITE_ATTR_SUBT) {
        for (size_t i = 0; i < ARRAYLEN(ap_sprite_subtypes); i++) {
            const struct ap_sprite_subtype *st= &ap_sprite_subtypes[i];
            if (st->only_dungeon_room != 0 && dungeon_room != st->only_dungeon_room) {
                continue;
            }
            if (st->type == type && st->subtype == subtype) {
                return st->attrs;
            }
        }
        return attrs & ~SPRITE_ATTR_SUBT;
    }
    return attrs;
}

void
ap_sprites_update() {
    ap_sprites_changed = false;

    for (size_t i = 0; i < 16; i++) {
        struct ap_sprite * sprite = &ap_sprites[i];
        if (ap_ram.sprite_type[i] != sprite->type) {
            ap_sprites_changed = true;
        }
    }
    if (ap_sprites_changed) {
        //LOGB("Sprites changed");
    }

    memset(ap_sprites, 0, sizeof ap_sprites);
    for (size_t i = 0; i < 16; i++) {
        struct ap_sprite * sprite = &ap_sprites[i];
        if (ap_ram.sprite_type[i] == 0)
            continue;

        sprite->type = ap_ram.sprite_type[i];
        sprite->state = ap_ram.sprite_state[i];
        sprite->subtype = ap_ram.sprite_subtype1[i] | (ap_ram.sprite_subtype2[i] << 8);
        sprite->interaction = ap_ram.sprite_interaction[i];
        sprite->hp = ap_ram.sprite_hp[i];
        sprite->drop = ap_ram.sprite_drop[i] == 0x03 ? 0 : ap_ram.sprite_drop[i];

        sprite->attrs = ap_sprite_attrs_for_type(sprite->type, sprite->subtype, *ap_ram.dungeon_room);
        bool spawned = ap_ram.sprite_spawned[i];

        switch (sprite->type) {
                /*
            case 0x41: // Guard-like sprites
            case 0x42:
            case 0x43:
            case 0x44:
            case 0x45:
            case 0x46:
            case 0x47:
            case 0x48:
            case 0x49:
            case 0x4a:
            case 0x4b:
            case 0x6a:
            case 0x18: // Moldorm
            case 0xa7: // Stalfos
            case 0x83: // Green Igor
            case 0x84: // Red Igor
            case 0x4e: // Curly thing
            case 0x4f: // Curly thing
                sprite->active = false;
                if ((sprite->interaction & 0x1F) && sprite->hp > 0) {
                    sprite->active = true;
                }
                break;
            case 0x53: // Armos Knights
                sprite->active = sprite->hp > 0; // && spawned;
                break;
                */
            default:
                if (sprite->attrs & SPRITE_ATTR_TALK) {
                    sprite->active = true;
                } else {
                    sprite->active = sprite->state != 0;
                }
                break;
        }

        sprite->hitbox_tl = XY(ap_ram.sprite_x_lo[i], ap_ram.sprite_y_lo[i]);
        sprite->hitbox_tl.x += ap_ram.sprite_x_hi[i] << 8u;
        sprite->hitbox_tl.y += ap_ram.sprite_y_hi[i] << 8u;

        if (*ap_ram.in_building) {
            sprite->hitbox_tl.x = ((sprite->hitbox_tl.x & ~0x1FF) << 2) | (sprite->hitbox_tl.x & 0x1FF);
            sprite->hitbox_tl.x += 0x4000;
            if (!ap_ram.sprite_lower_level[i] && ap_ram.sprite_type[i] != 0x50) {
                sprite->hitbox_tl.x += 0x200;
            }
        }

        uint8_t h = ap_ram.sprite_hitbox_idx[i] & 0x1Fu;
        sprite->tl = sprite->hitbox_tl;
        sprite->tl.x += ap_ram.hitbox_x_lo[h];
        sprite->tl.x += ap_ram.hitbox_x_hi[h] << 8u;
        sprite->tl.y += ap_ram.hitbox_y_lo[h];
        sprite->tl.y += ap_ram.hitbox_y_hi[h] << 8u;

        sprite->br = sprite->tl;
        sprite->br.x += ap_ram.hitbox_width[h];
        sprite->br.y += ap_ram.hitbox_height[h];

        if (sprite->attrs & SPRITE_ATTR_BLKF) {
            assert(!(sprite->attrs & SPRITE_ATTR_BLKS));
            sprite->hitbox_tl = sprite->tl;
            sprite->hitbox_br = sprite->br;
        } else if (sprite->attrs & SPRITE_ATTR_BLKS) {
            sprite->hitbox_br = XYOP1(sprite->hitbox_tl, +15);
        } else {
            sprite->hitbox_tl = XY(0, 0);
            sprite->hitbox_br = XY(0, 0);
        }
        if (sprite->type == 0x40) {
            // Hyrule Castle Barrier to Agahnim's Tower
            sprite->hitbox_br.y += 16;
        }
    }

    // The blocks don't seem to update in real-time so not sure if this
    // memory region is actually useful
    memset(ap_pushblocks, 0, sizeof(ap_pushblocks));
    size_t n_blocks = 0;
    if (*ap_ram.in_building) {
        struct xy raw_room_tl;
        raw_room_tl = XYOP1(ap_link_xy(), & (uint16_t) ~0x1FF);
        for (size_t i = 0; i < (0x18C / 4); i++) {
            uint16_t room_id = ap_ram.block_data[i] & 0xFFFF;
            if (room_id != *ap_ram.dungeon_room) {
                continue;
            }
            uint16_t tile_addr = ap_ram.block_data[i] >> 16;
            struct xy xy = XY((tile_addr & 0x7E) << 2, (tile_addr & 0x1F80) >> 4);
            //LOG("Raw block %#x: %#x %#x", tile_addr, xy.x, xy.y);
            xy = XYOP2(raw_room_tl, +, xy);
            //struct xy xy = ap_map16_to_xy(raw_room_tl, tile_addr & 0x1FFF);
            if (tile_addr & 0x2000) {
                xy = XYFLIPBG(xy); // XXX test this
            }
            ap_pushblocks[n_blocks++].tl = xy;
            assert(n_blocks < ARRAYLEN(ap_pushblocks));
            //LOGB("> Block at %#x " PRIXYV, tile_addr, PRIXYVF(xy));
        }
    }
}

void
ap_sprites_print()
{
    struct xy link = ap_link_xy();
    for (uint8_t i = 0; i < 16; i++) {
        if (ap_sprites[i].type == 0)
            continue;
        LOG("Sprite %-2u: %c attrs=%s type=%#-4x subtype=%#-6x state=%#-4x inter=%#-4x hp=%-2d drop=%#-4x " PRIBBV " dist=%d",
            i, "qA"[!!ap_sprites[i].active], ap_sprite_attr_name(ap_sprites[i].attrs),
            ap_sprites[i].type, ap_sprites[i].subtype, ap_sprites[i].state,
            ap_sprites[i].interaction, ap_sprites[i].hp, ap_sprites[i].drop,
            PRIBBVF(ap_sprites[i]), XYL1BOXDIST(link, ap_sprites[i].tl, ap_sprites[i].br));
    }
    //LOG("Overlord %#x timer: %#x", ap_ram.overlord_types[7], ap_ram.overlord_timers[7]);

    // This only works inside
    // The blocks don't seem to update in real-time so not sure if this
    // memory region is actually useful
    for (size_t i = 0; i < ARRAYLEN(ap_pushblocks); i++) {
        if (XYEQ(ap_pushblocks[i].tl, XY(0, 0))) {
            continue;
        }
        LOG("Pushblock: " PRIXYV, PRIXYVF(ap_pushblocks[i].tl));
    }
}

const char * ap_door_attr_name(uint16_t idx) {
    uint16_t attr = ap_door_attrs[idx];
    static char sbuf[1024];
    char *buf = sbuf;
    if (!attr) {
        sprintf(buf, "unknown:%#x", idx);
        return sbuf;
    }

#define X(d) if (attr & CONCAT(DOOR_ATTR_, d)) { \
    if (buf != sbuf) { *buf++ = '|'; } \
    buf += sprintf(buf, STRINGIFY(d)); \
    }
DOOR_ATTR_LIST
#undef X
    return sbuf;
}

const char * const ap_inventory_names[] = {
#define X(i, n) [i] = STRINGIFY(n),
INVENTORY_LIST
#undef X
};

struct ap_ancillia ap_ancillia[N_ANCILLIA];

void
ap_ancillia_update() {
    for (size_t i = 0; i < N_ANCILLIA; i++) {
        struct ap_ancillia * ancillia = &ap_ancillia[i];
        ancillia->type = ap_ram.ancillia_type[i];
        ancillia->attrs = ap_ancillia_attrs[ancillia->type];
        ancillia->bf0 = ap_ram.ancillia_bf0[i];

        ancillia->tl = XY(ap_ram.ancillia_x_lo[i], ap_ram.ancillia_y_lo[i]);
        ancillia->tl.x += ap_ram.ancillia_x_hi[i] << 8u;
        ancillia->tl.y += ap_ram.ancillia_y_hi[i] << 8u;
        if (*ap_ram.in_building) {
            ancillia->tl.x = ((ancillia->tl.x & ~0x1FF) << 2) | (ancillia->tl.x & 0x1FF);
            ancillia->tl.x += 0x4000;
            if (!ap_ram.ancillia_lower_level[i]) {
                ancillia->tl.x += 0x200;
            }
        }
        ancillia->br = XYOP1(ancillia->tl, +15);

        ancillia->subpixel = XY(ap_ram.ancillia_x_sub[i], ap_ram.ancillia_y_sub[i]);
        ancillia->velocity = XY(ap_ram.ancillia_x_vel[i], ap_ram.ancillia_y_vel[i]);
    }
}

void
ap_ancillia_print() {
    const struct xy link = ap_link_xy();
    bool printed_any = false;
    for (size_t i = 0; i < N_ANCILLIA; i++) {
        const struct ap_ancillia * ancillia = &ap_ancillia[i];
        if (ancillia->type == 0) {
            continue;
        }
        LOG("Ancillia %zu: type=%#x bf0=%#x " PRIXYV " dist=%d",
            i, ancillia->type, ancillia->bf0,
            PRIXYVF(ancillia->tl), XYL1DIST(link, ancillia->tl));
        printed_any = true;
    }
    if (!printed_any) {
        LOG("No Ancillia");
    }
}

const char *
ap_quadrant_print(uint8_t quadmask) {
    static char sbuf[8];
    char *buf = sbuf;
    if (quadmask & QUAD_A) *buf++ = 'A';
    if (quadmask & QUAD_B) *buf++ = 'B';
    if (quadmask & QUAD_C) *buf++ = 'C';
    if (quadmask & QUAD_D) *buf++ = 'D';
    *buf++ = '\0';
    return sbuf;
}

const char * ap_room_tag_print(const struct ap_room_tag * tag) {
    if (tag == NULL) {
        return "";
    }

    const char *action = "(unknown)";
    switch(tag->action) {
#define X(n) case CONCAT(ROOM_ACTION_, n): action = STRINGIFY(n); break;
ROOM_ACTION_LIST
#undef X
    }

    const char *result = "(unknown)";
    switch(tag->result) {
#define X(n) case CONCAT(ROOM_RESULT_, n): result = STRINGIFY(n); break;
ROOM_RESULT_LIST
#undef X
    }

    static char sbuf[256];
    snprintf(sbuf, sizeof(sbuf), "%s:%s:%s",
            ap_quadrant_print(tag->quadmask), action, result);
    return sbuf;
}
