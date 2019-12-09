#ifndef __AP_REQ__
#define __AP_REQ__

#include "ap_snes.h"
#include <assert.h>
#include <stdbool.h>

#define AP_REQ_SLOTS ((size_t) 4)

struct ap_reqmask {
    uint64_t bits;
};

struct ap_req {
    struct ap_reqmask alternative_requirements[AP_REQ_SLOTS];
};

// Call every frame
void ap_req_update(void);

void ap_req_init(struct ap_req * req);

void ap_req_require(struct ap_req * req, size_t slot, uint16_t requirement);

bool ap_req_is_satisfied(const struct ap_req * req);

void ap_req_print(const struct ap_req * req, char *buf);

#define REQUIREMENT_LIST \
    INVENTORY_LIST \
    X(0, MASTER_SWORD) \

enum ap_requirement {
#define X(i, n) CONCAT(REQUIREMENT_, n),
REQUIREMENT_LIST
#undef X
    _REQUIREMENT_MAX
};
static_assert(_REQUIREMENT_MAX < 64, "Reimplement reqmask to support >= 64 requirements");

extern const char * const ap_requirement_names[];

#endif
