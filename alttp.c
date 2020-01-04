#include "alttp_public.h"
#include "ap_macro.h"
#include "ap_map.h"
#include "ap_math.h"
#include "ap_snes.h"
#include "ap_plan.h"
#include "ap_req.h"

void
ap_tick(uint32_t frame, uint16_t * joypad) {
    assert_bp(frame == 0 || frame == ap_frame + 1);
    ap_frame = frame;

    *ap_emu->info_string_ptr = ap_info_string;
    ap_sprites_update();
    ap_ancillia_update();

    // Cheating!
    //*(uint8_t *) (uintptr_t) ap_ram.ignore_sprites = 0xFF;
    *(uint8_t *) (uintptr_t) ap_ram.health_current = *ap_ram.health_capacity;
    *(uint8_t *) (uintptr_t) ap_ram.inventory_bombs = 10;
    *(uint8_t *) (uintptr_t) ap_ram.inventory_quiver = 10;
    //*(uint8_t *) (uintptr_t) ap_ram.inventory_gloves = 1;
    *(uint8_t *) (uintptr_t) &ap_ram.inventory_base[INVENTORY_LAMP] = 1;

    ap_req_update();

    static bool has_imported = false;
    if (!has_imported) {
        LOG("importing");
        ap_map_import("map.19.txt");
        //ap_map_import("map_state.5.00019532.txt");
        //ap_map_import("map_state.7.00021016.txt");
        has_imported = true;
    } else if (frame == 100 && false) {
        ap_map_export("map_state_reflect.txt");
        ap_print_state();
        ap_print_map_full();
        //ap_graph_print();
        LOGB("Current requirements satisfied: %s", ap_req_print(NULL));
        ap_print_map_screen(NULL);
        //ap_print_goals();
        //exit(0);
    }

    if (ap_manual_mode) {
        return;
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
    //INFO("Module: %#x; Submodule: %#x; %d", module_index, submodule_index, frame);
    switch (module_index) {
    case 0x07:
    case 0x09:
    case 0x0B:
        if (submodule_index == 0x08 || submodule_index == 0x10) {
            // Stairs; keep following targets (even though we can't change direction);
            ap_follow_targets(joypad);
        }
        if (submodule_index != 0x00) {
            return;
        }
        break;
        //if (frame % 2) JOYPAD_SET(START);
        // fallthrough
    case 0x0E:
        break;
    default:
        return;
    }
    if (*ap_ram.crystal_timer != 0) {
        INFO("Waiting for crystal to drop (%d)", *ap_ram.crystal_timer);
        *joypad = 0;
        return;
    }

    struct xy topleft, bottomright;
    ap_map_bounds(&topleft, &bottomright);
    struct xy link = ap_link_xy();

    ap_debug = JOYPAD_TEST(START);
    ap_map_tick();
    if (*ap_ram.link_state == LINK_STATE_HOLDING_BIG_ROCK) {
        JOYPAD_MASH(A, 0x01);
    } else if (*ap_ram.link_state == LINK_STATE_RECVING_ITEM || *ap_ram.link_state == LINK_STATE_RECVING_ITEM2) {
        JOYPAD_MASH(A, 0x01);
    } else if (*ap_ram.link_state == LINK_STATE_FALLING_HOLE && *ap_ram.link_falling >= 2) {
        INFO("Falling into a hole...");
    } else if (*ap_ram.link_state != LINK_STATE_GROUND
            && *ap_ram.link_state != LINK_STATE_FALLING_HOLE) {
        INFO("Link state: %#x", *ap_ram.link_state);
    } else {
        if (!JOYPAD_TEST(Y)) {
            ap_plan_evaluate(joypad);
        }
    }

    static uint16_t x = 0;
    if (x++ == 40000) { // && false) {
        //ap_print_map_full();
        //ap_graph_print();
        LOGB("Current requirements satisfied: %s", ap_req_print(NULL));
        char filename[128];
        snprintf(filename, sizeof filename, "map_state.8.%08u.txt", frame);
        //ap_map_export(filename);
        x = 0;
    }

    //LOG("Link %d %d %u %u", *ap_ram.link_dx, *ap_ram.link_dy, *ap_ram.link_x, *ap_ram.link_y);
    //INFO("citem: %#x", *ap_ram.current_item);

    //INFO_HEXDUMP(b);
    if (frame % 2) {
        //JOYPAD_SET(A);
    }
}
