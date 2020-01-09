#include "ap_item.h"
#include "lb.h"

// items x item_locs
static struct lb item_lb;

static void
ap_item_init() {
    lb_init(&item_lb, N_ITEMS);
}

void
ap_item_update() {
    static bool initialized = false;
    if (!initialized) {
        ap_item_init();
        initialized = true;
    }


}

void
ap_item_print_state() {

}
