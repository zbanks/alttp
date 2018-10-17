#include "alttp_public.h"
#include "ap_macro.h"
#include "ap_map.h"
#include "ap_math.h"
#include "ap_snes.h"
#include "ap_plan.h"

void
ap_tick(uint32_t frame, uint16_t * joypad) {
    *ap_emu->info_string_ptr = ap_info_string;

    if (JOYPAD_EVENT(START)) {
        JOYPAD_CLEAR(START);
        if (frame > 60)
            ap_emu->save("home");
    }

    struct xy topleft, bottomright;
    ap_map_bounds(&topleft, &bottomright);
    struct xy link = ap_link_xy();

    //INFO("a: %d b: %d", *(uint16_t *)ap_emu->base(0x7E0410), *(uint16_t *)ap_emu->base(0x7E0412));

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

    ap_debug = JOYPAD_TEST(START);

    ap_update_map_node();
    ap_plan_evaluate(joypad);
    //ap_print_map();

    JOYPAD_CLEAR(START);

    if (!JOYPAD_TEST(TR)) {
        //ap_follow_targets(joypad);
    }

    //INFO("L:" PRIXY " M: " PRIXY "," PRIXY, PRIXYF(ap_link_xy()), PRIXYF(topleft), PRIXYF(bottomright));


    if (JOYPAD_EVENT(Y)) {
        ap_print_map();
    }



    //uint8_t *b = ap_emu->base(0x7E001A);
    //INFO_HEXDUMP(b);
    if (frame % 2) {
        //JOYPAD_SET(A);
    }

    // Cheating!
    *(uint8_t *) (uintptr_t) ap_ram.ignore_sprites = 0xFF;
}
