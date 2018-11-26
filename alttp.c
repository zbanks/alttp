#include "alttp_public.h"
#include "ap_macro.h"
#include "ap_map.h"
#include "ap_math.h"
#include "ap_snes.h"
#include "ap_plan.h"

void
ap_tick(uint32_t frame, uint16_t * joypad) {
    *ap_emu->info_string_ptr = ap_info_string;

    if (JOYPAD_TEST(X)) return;
    /*
    if (JOYPAD_EVENT(START)) {
        JOYPAD_CLEAR(START);
        if (frame > 60) ap_emu->save("home");
    }
    */
// 0x07 In house/dungeon
// 0x09 In OW
// 0x0B Overworld Mode (special overworld)
    uint8_t module_index = *ap_ram.module_index;
    switch (module_index) {
    case 0x07:
    case 0x09:
    case 0x0B:
        break;
    case 0x0E:
        if (frame % 4 == 0) {
            JOYPAD_SET(START);
            JOYPAD_SET(B);
        } else {
            JOYPAD_CLEAR(START);
            JOYPAD_CLEAR(B);
        }
        // fallthrough
    default:
        INFO("Module Index: %#x", module_index);
        return;
    }

    struct xy topleft, bottomright;
    ap_map_bounds(&topleft, &bottomright);
    struct xy link = ap_link_xy();

    //INFO("a: %d b: %d", *(uint16_t *)ap_emu->base(0x7E0410), *(uint16_t *)ap_emu->base(0x7E0412));

    static uint16_t last_map = -1; 
    static uint16_t transition_counter = 0;
    if (last_map == XYMAPSCREEN(topleft)) {
        transition_counter++;
    } else {
        last_map = XYMAPSCREEN(topleft);
        transition_counter = 0;
    }
    if (transition_counter < 300 || !XYIN(link, topleft, bottomright)) {
        // Swing our sword for 300 frames, or until we can move
        if (transition_counter & 1) {
            JOYPAD_SET(B);
        } else {
            JOYPAD_CLEAR(B);
        }
        if ((*ap_ram.link_swordstate & 0xFE) == 0x80) {
            transition_counter = 300;
        } else {
            return;
        }
    }

    ap_debug = JOYPAD_TEST(START);
    ap_update_map_screen(false);
    if (*ap_ram.link_state != LINK_STATE_GROUND) {
        //LOG("Link state: %#x", *ap_ram.link_state);
    } else {
        //LOG("touching_chest: %d", *ap_ram.touching_chest)g;
        if (!JOYPAD_TEST(Y)) {
            //ap_plan_evaluate(joypad);
        }
    }

    //JOYPAD_CLEAR(START);

    if (!JOYPAD_TEST(TR)) {
        //ap_follow_targets(joypad);
    }

    //INFO("%u L:" PRIXY " M: " PRIXY "," PRIXY " m %u", *ap_ram.dungeon_room, PRIXYF(ap_link_xy()), PRIXYF(topleft), PRIXYF(bottomright), XYMAPSCREEN(topleft));

    static uint16_t x = 0;
    if (x++ == 4000) {
        ap_print_map_full();
        x = 0;
    }

    if (JOYPAD_TEST(Y)) {
        //ap_print_map_screen(NULL);
    }

    // sram_overworld_state & 0x2 = bomb open

    if (*ap_ram.in_building) {
        INFO("Room: %x St: %4x SR: %4x", *ap_ram.dungeon_room, *ap_ram.room_state, ap_ram.sram_room_state[*ap_ram.dungeon_room]);
    } else {
        INFO("Ovrwld: %x St: %2x", *ap_ram.overworld_index, ap_ram.sram_overworld_state[*ap_ram.overworld_index]);
    }

    //uint8_t *b = ap_emu->base(0x7E001A);
    //INFO_HEXDUMP(b);
    if (frame % 2) {
        //JOYPAD_SET(A);
    }

    // Cheating!
    *(uint8_t *) (uintptr_t) ap_ram.ignore_sprites = 0xFF;
    *(uint8_t *) (uintptr_t) ap_ram.health_current = *ap_ram.health_capacity;
    *(uint8_t *) (uintptr_t) ap_ram.inventory_bombs = 10;
}
