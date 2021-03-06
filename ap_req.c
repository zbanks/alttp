#include "ap_req.h"
#include "bm.h"

const char * const ap_requirement_names[] = {
#define X(i, n) [CONCAT(REQUIREMENT_, n)] = STRINGIFY(n),
REQUIREMENT_LIST
#undef X
    NULL,
};

static struct ap_reqmask current_reqmask = {0};

static void reqmask_set(struct ap_reqmask * mask, enum ap_requirement req) {
    assert(req < _REQUIREMENT_MAX);
    BM_SET(mask->bits, req);
}

static bool reqmask_match(const struct ap_reqmask * src) {
    if (BM_ISEMPTY(src->bits)) return false;
    return BM_ISMATCH(src->bits, current_reqmask.bits);
}

static void reqmask_print(const struct ap_reqmask * mask, char **buf_p) {
    char * buf = *buf_p;
    if (BM_ISEMPTY(mask->bits)) {
        buf += sprintf(buf, "None");
    } else {
        bool first = true;
        for (int i = 0; i < _REQUIREMENT_MAX; i++) {
            if (BM_ISSET(mask->bits, i)) {
                if (first) {
                    first = false;
                } else {
                    *buf++ = '&';
                }
                buf += sprintf(buf, "%s", ap_requirement_names[i]);
            }
        }
    }
    *buf_p = buf;
}

void ap_req_update() {
    memset(&current_reqmask, 0, sizeof(current_reqmask));
#define X(i, n) \
    if (i != 0 && ap_ram.inventory_base[i] != 0) { \
        reqmask_set(&current_reqmask, CONCAT(REQUIREMENT_, n)); \
    }
    REQUIREMENT_LIST
#undef X
    if (*ap_ram.inventory_sword > 1) {
        reqmask_set(&current_reqmask, REQUIREMENT_MASTER_SWORD);
    }
    if (*ap_ram.dungeon_current_keys> 0) {
        // XXX how to handle this across dungeons
        reqmask_set(&current_reqmask, REQUIREMENT_KEY);
    }
    if (*ap_ram.sram_pendants & 0x01) {
        reqmask_set(&current_reqmask, REQUIREMENT_GREEN_PENDANT);
    }
    if ((*ap_ram.sram_pendants & 0x07) == 0x07) {
        reqmask_set(&current_reqmask, REQUIREMENT_ALL_PENDANTS);
    }
    if ((*ap_ram.sram_crystals & 0x05) == 0x05) {
        reqmask_set(&current_reqmask, REQUIREMENT_FIVESIX_CRYSTALS);
    }
    if ((*ap_ram.sram_crystals & 0x7F) == 0x7F) {
        reqmask_set(&current_reqmask, REQUIREMENT_ALL_CRYSTALS);
    }
    if (*ap_ram.inventory_gloves >= 1) {
        reqmask_set(&current_reqmask, REQUIREMENT_GLOVES_1);
    }
    if (*ap_ram.inventory_gloves >= 2) {
        reqmask_set(&current_reqmask, REQUIREMENT_GLOVES_2);
    }
}

void ap_req_init(struct ap_req * req) {
    memset(req, 0x00, sizeof(*req));
}

void ap_req_require(struct ap_req * req, size_t slot, uint16_t requirement) {
    assert(slot < AP_REQ_SLOTS);
    assert(requirement < (uint16_t) _REQUIREMENT_MAX);
    reqmask_set(&req->alternative_requirements[slot], requirement);
}

bool ap_req_is_satisfied(const struct ap_req * req) {
    bool all_zero = true;
    for (size_t i = 0; i < AP_REQ_SLOTS; i++) {
        if (reqmask_match(&req->alternative_requirements[i])) {
            return true;
        }
        if (!BM_ISEMPTY(req->alternative_requirements[i].bits)) {
            all_zero = false;
        }
    }
    return all_zero;
}

const char * ap_req_print(const struct ap_req * req) {
    static char sbuf[4096];
    char *buf = sbuf;
    if (req == NULL) {
        reqmask_print(&current_reqmask, &buf);
        return sbuf;
    }

    *buf++ = '[';
    bool first = true;
    for (size_t i = 0; i < AP_REQ_SLOTS; i++) {
        if (BM_ISEMPTY(req->alternative_requirements[i].bits)) {
            continue;
        }
        if (first) {
            first = false;
        } else {
            buf += sprintf(buf, " | ");
        }
        reqmask_print(&req->alternative_requirements[i], &buf);
    }
    if (first) {
        buf += sprintf(buf, "Any");
    }
    *buf++ = ']';
    *buf++ = '\0';
    return sbuf;
}
