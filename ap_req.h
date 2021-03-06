#ifndef __AP_REQ__
#define __AP_REQ__

#include "ap_snes.h"
#include <assert.h>
#include <stdbool.h>

#define AP_REQ_SLOTS ((size_t) 4)

struct ap_reqmask {
    uint64_t bits[1];
};

struct ap_req {
    struct ap_reqmask alternative_requirements[AP_REQ_SLOTS];
};

// Call every frame
void ap_req_update(void);

void ap_req_init(struct ap_req * req);

void ap_req_require(struct ap_req * req, size_t slot, uint16_t requirement);

bool ap_req_is_satisfied(const struct ap_req * req);

const char * ap_req_print(const struct ap_req * req);

#define REQUIREMENT_LIST \
    INVENTORY_LIST \
    X(0, MASTER_SWORD) \
    X(0, KEY) \
    X(0, GREEN_PENDANT) \
    X(0, ALL_PENDANTS) \
    X(0, FIVESIX_CRYSTALS) \
    X(0, ALL_CRYSTALS) \
    X(0, GLOVES_1) \
    X(0, GLOVES_2) \

enum ap_requirement {
#define X(i, n) CONCAT(REQUIREMENT_, n),
REQUIREMENT_LIST
#undef X
    _REQUIREMENT_MAX
};
static_assert(_REQUIREMENT_MAX < 64, "Reimplement reqmask to support >= 64 requirements");

extern const char * const ap_requirement_names[];

#endif
