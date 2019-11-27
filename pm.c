#include "pm.h"
#include <stdlib.h>
#include <stdbool.h>

struct pm {
    size_t values_count;
    size_t values_capacity;
    size_t refs_count;
    size_t refs_capacity;
    struct pm_kv {
        uintptr_t key;
        union {
            void * value;
            void ** value_p;
        };
    } * values;
    struct pm_kv * refs;
};

struct pm * pm_create() {
    struct pm * pm = calloc(1, sizeof *pm);
    if (pm == NULL) return NULL;

    *pm = (struct pm) {
        .values_capacity = 0,
        .refs_capacity = 0,
        .values_count = 0,
        .refs_count = 0,
        .values = NULL,
        .refs = NULL,
    };
    return pm;
}

static int pm_kv_cmp(const void * _a, const void * _b) {
    const struct pm_kv * a = _a;
    const struct pm_kv * b = _b;
    if (a->key < b->key) return -1;
    if (a->key > b->key) return 1;
    return 0;
}

size_t pm_destroy(struct pm * pm) {
    qsort(pm->values, pm->values_count, sizeof *pm->values, &pm_kv_cmp);
    //qsort(pm->refs, pm->refs, sizeof *pm->refs, &pm_kv_cmp);
    size_t unmatched = 0;
    for (size_t i = 0; i < pm->refs_count; i++) {
        struct pm_kv * ref = &pm->refs[i];
        struct pm_kv * value = bsearch(ref, pm->values, pm->values_count, sizeof *pm->values, &pm_kv_cmp);
        if (value == NULL) {
            unmatched++;
            continue;
        }
        *ref->value_p = value->value;
    }

    free(pm->values);
    free(pm->refs);
    free(pm);
    return unmatched;
}

int pm_set(struct pm * pm, uintptr_t key, void * value) {
    if (pm->values_count >= pm->values_capacity) {
        size_t new_capacity = pm->values_capacity * 2;
        if (new_capacity < 16) new_capacity = 16;
        void * new_values = realloc(pm->values, new_capacity * sizeof(*pm->values));
        if (new_values == NULL) return -1;
        pm->values_capacity = new_capacity;
        pm->values = new_values;
    }
    pm->values[pm->values_count++] = (struct pm_kv) {
        .key = key,
        .value = value,
    };
    return 0;
}

int pm_get(struct pm * pm, uintptr_t key, void ** value_p) {
    if (pm->refs_count >= pm->refs_capacity) {
        size_t new_capacity = pm->refs_capacity * 2;
        if (new_capacity < 16) new_capacity = 16;
        void * new_refs = realloc(pm->refs, new_capacity * sizeof(*pm->refs));
        if (new_refs == NULL) return -1;
        pm->refs_capacity = new_capacity;
        pm->refs = new_refs;
    }
    pm->refs[pm->refs_count++] = (struct pm_kv) {
        .key = key,
        .value_p = value_p,
    };
    return 0;
}
