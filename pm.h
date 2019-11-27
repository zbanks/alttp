#ifndef __PM_H__
#define __PM_H__

#include <stddef.h>
#include <stdint.h>

// Pointer map (sorted list)

struct pm;

struct pm * pm_create(void);
size_t pm_destroy(struct pm * pm);

int pm_set(struct pm * pm, uintptr_t key, void * value);
int pm_get(struct pm * pm, uintptr_t key, void ** value_p);

#endif

