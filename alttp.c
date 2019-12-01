#include "alttp_public.h"
#include "ap_macro.h"
#include "ap_map.h"
#include "ap_math.h"
#include "ap_snes.h"
#include "ap_plan.h"

void
ap_tick(uint32_t frame, uint16_t * joypad) {
    *ap_emu->info_string_ptr = ap_info_string;
    ap_sprites_update();

    // Cheating!
    *(uint8_t *) (uintptr_t) ap_ram.ignore_sprites = 0xFF;
    *(uint8_t *) (uintptr_t) ap_ram.health_current = *ap_ram.health_capacity;
    *(uint8_t *) (uintptr_t) ap_ram.inventory_bombs = 10;

    static bool has_imported = false;
    if (!has_imported) {
        LOG("importing");
        ap_map_import("map_state.5.00019532.txt");
        has_imported = true;
    } else if (frame == 100 && false) {
        ap_map_export("map_state_reflect.txt");
        ap_print_state();
        ap_print_map_full();
        ap_graph_print();
        ap_print_map_screen(NULL);
        //ap_print_goals();
        //exit(0);
    }

    if (JOYPAD_EVENT(Y)) {
        LOG("dumping");
        ap_print_map_screen(NULL);
    }
    if (JOYPAD_TEST(X)) return;
    /*
    if (JOYPAD_EVENT(START)) {
        JOYPAD_CLEAR(START);
        if (frame > 60) ap_emu->save("home");
    }
    */
    *joypad = 0;

// 0x07 In house/dungeon
// 0x09 In OW
// 0x0B Overworld Mode (special overworld)
// 0x0E Dialog or Inventory
    uint8_t module_index = *ap_ram.module_index;
    uint8_t submodule_index = *ap_ram.submodule_index;
    //LOG("Module: %x; Submodule: %x", module_index, submodule_index);
    INFO("Module: %#x; Submodule: %#x; %d", module_index, submodule_index, frame);
    switch (module_index) {
    case 0x07:
    case 0x09:
    case 0x0B:
        if (submodule_index != 0x00 && submodule_index != 0x10) return;
        break;
        //if (frame % 2) JOYPAD_SET(START);
        // fallthrough
    case 0x0E:
        break;
    default:
        return;
    }

    struct xy topleft, bottomright;
    ap_map_bounds(&topleft, &bottomright);
    struct xy link = ap_link_xy();

    //INFO("a: %d b: %d", *(uint16_t *)ap_emu->base(0x7E0410), *(uint16_t *)ap_emu->base(0x7E0412));

    // May be able to use submodule index (0x1, 0xA) to find transitions
    /*
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
    */

    ap_debug = JOYPAD_TEST(START);
    struct ap_screen * screen = ap_update_map_screen(false);
    if (*ap_ram.link_state != LINK_STATE_GROUND) {
        //LOG("Link state: %#x", *ap_ram.link_state);
    } else {
        //LOG("touching_chest: %d", *ap_ram.touching_chest)g;
        if (!JOYPAD_TEST(Y)) {
            ap_plan_evaluate(joypad);
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
        ap_graph_print();
        char filename[128];
        snprintf(filename, sizeof filename, "map_state.6.%08u.txt", frame);
        ap_map_export(filename);
        x = 0;
    }

    // sram_overworld_state & 0x2 = bomb open

    /*
    if (*ap_ram.in_building) {
        INFO("%X Room: %#06x St: %4x SR: %4x      %s", module_index, screen->id, *ap_ram.room_state, ap_ram.sram_room_state[*ap_ram.dungeon_room], screen->name);
    } else {
        INFO("%X Ovrwld: %#06x St: %2x               %s", module_index, screen->id, ap_ram.sram_overworld_state[*ap_ram.overworld_index], screen->name);
    }
    */
    //LOG("Link %d %d %u %u", *ap_ram.link_dx, *ap_ram.link_dy, *ap_ram.link_x, *ap_ram.link_y);
    //INFO("citem: %#x", *ap_ram.current_item);

    //uint8_t *b = ap_emu->base(0x7E001A);
    //INFO_HEXDUMP(b);
    if (frame % 2) {
        //JOYPAD_SET(A);
    }

}
