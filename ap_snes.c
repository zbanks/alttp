#include "ap_snes.h"
#include "ap_macro.h"
#include "ap_math.h"
#include "ap_map.h"
#include "ap_plan.h"

struct ap_ram ap_ram;
struct ap_snes9x * ap_emu = NULL;

char ap_info_string[INFO_STRING_SIZE];
bool ap_debug;

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

const char * ap_sprite_attr_name(uint8_t idx) {
    uint16_t attr = ap_sprite_attrs[idx];
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
    ap_emu->load("well");
    //ap_emu->load("castle");
    //ap_emu->load("estpal");
    ap_emu->load("home");

    ap_plan_init();
}

void
ap_sprites_print()
{
    struct xy link = ap_link_xy();
    for (uint8_t i = 0; i < 16; i++) {
        if (ap_ram.sprite_type[i] == 0)
            continue;
        uint16_t sprite_attrs = ap_sprite_attrs[ap_ram.sprite_type[i]];
        //if (!(sprite_attrs & (SPRITE_ATTR_SWCH | SPRITE_ATTR_TALK | SPRITE_ATTR_FLLW | SPRITE_ATTR_ITEM)))
        //    continue;

        struct xy sprite_tl = ap_sprite_xy(i);
        LOG("Sprite %-2u: attrs=%s type=%#x subt1=%#x subt2=%#x state=%#x" PRIXYV " dist=%d",
            i, ap_sprite_attr_name(ap_ram.sprite_type[i]), 
            ap_ram.sprite_type[i], ap_ram.sprite_subtype1[i],
            ap_ram.sprite_subtype2[i], ap_ram.sprite_state[i],
            PRIXYVF(sprite_tl), XYL1DIST(link, sprite_tl));
    }
}
