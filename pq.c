#include "pq.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct pq_entry {
    uint64_t priority;
    unsigned char data[];
};

struct pq {
    size_t data_size;
    size_t capacity;
    size_t size;
    unsigned char * entries; // Array of `struct pq_entry`
};

#define PQ_PARENT(I)    (I == 0 ? 0 : ((I - 1) / 2))
#define PQ_LEFT(I)      (I * 2 + 1)
#define PQ_RIGHT(I)     (I * 2 + 2)

#define PQ_ENTRY(PQ, I)  ((struct pq_entry *) &(PQ)->entries[I * PQ_ENTRY_SIZE(PQ)])
#define PQ_ENTRY_SIZE(PQ) (sizeof(uint64_t) + (PQ)->data_size)

struct pq *
pq_create(size_t data_size) {
    struct pq * pq = calloc(1, sizeof *pq);
    if (pq == NULL) {
        return NULL;
    }

    pq->data_size = data_size;
    pq->capacity = 1024;
    pq->size = 0;
    pq->entries = calloc(pq->capacity, PQ_ENTRY_SIZE(pq));
    if (pq->entries == NULL) {
        return free(pq), NULL;
    }

    return pq;
}

void
pq_destroy(struct pq * pq) {
    free(pq->entries);
    free(pq);
}

void
pq_clear(struct pq * pq) {
    pq->size = 0;

#ifdef PQ_DEBUG
    memset(pq->entries, 0, PQ_ENTRY_SIZE(pq) * pq->capacity);
#endif
}

int
pq_push(struct pq * pq, uint64_t priority, const void * data) {
    size_t idx = pq->size++;

    if (pq->size > pq->capacity) {
        size_t new_capacity = pq->capacity * 2;
        void * new_entries = realloc(pq->entries, new_capacity * PQ_ENTRY_SIZE(pq));
        if (new_entries == NULL) {
            return -1;
        }
        pq->entries = new_entries;
        memset(PQ_ENTRY(pq, pq->capacity), 0, PQ_ENTRY_SIZE(pq) * pq->capacity);
        pq->capacity = new_capacity;
    }

    struct pq_entry * entry = PQ_ENTRY(pq, idx);
    while (idx > 0) {
        size_t parent_idx = PQ_PARENT(idx);
        struct pq_entry * parent = PQ_ENTRY(pq, parent_idx);
        if (parent->priority <= priority) {
            break;
        }

        memcpy(entry, parent, PQ_ENTRY_SIZE(pq));
        idx = parent_idx;
        entry = parent;
    }

    entry->priority = priority;
    if (data != NULL) {
        memcpy(entry->data, data, pq->data_size);
    }

    return 0;
}

int
pq_pop(struct pq * pq, uint64_t * priority_out, void * data_out) {
    if (pq->size == 0)
        return -1;

    size_t idx = 0;
    struct pq_entry * entry = PQ_ENTRY(pq, idx);
    if (priority_out != NULL) {
        *priority_out = entry->priority;
    }
    if (data_out != NULL) {
        memcpy(data_out, entry->data, pq->data_size);
    }

    size_t child_idx = --pq->size;
    uint64_t priority = PQ_ENTRY(pq, child_idx)->priority;
    while (idx < pq->size) {
        size_t left_idx = PQ_LEFT(idx); 
        size_t right_idx = PQ_RIGHT(idx); 

        size_t smallest_idx = idx;
        size_t smallest_priority = priority;
        if (left_idx < pq->size) {
            struct pq_entry * left_entry = PQ_ENTRY(pq, left_idx);
            if (left_entry->priority < smallest_priority) {
                smallest_priority = left_entry->priority;
                smallest_idx = left_idx;
            }
        }
        if (right_idx < pq->size) {
            struct pq_entry * right_entry = PQ_ENTRY(pq, right_idx);
            if (right_entry->priority < smallest_priority) {
                smallest_priority = right_entry->priority;
                smallest_idx = right_idx;
            }
        }
        if (smallest_idx == idx) {
            break;
        }
        memcpy(PQ_ENTRY(pq, idx), PQ_ENTRY(pq, smallest_idx), PQ_ENTRY_SIZE(pq));
        idx = smallest_idx;
    }
    if (child_idx != idx) {
        memcpy(PQ_ENTRY(pq, idx), PQ_ENTRY(pq, child_idx), PQ_ENTRY_SIZE(pq));
    }

#ifdef PQ_DEBUG
    for (size_t i = 0; i < pq->size; i++) {
        entry = PQ_ENTRY(pq, i);
        if (entry->priority < *priority_out)
            printf("Found entry with priority %zu < %zu\n", entry->priority, *priority_out);
    }
#endif

    return 0;
}

size_t
pq_size(struct pq * pq) {
    return pq->size;
}

#ifdef PQ_DEBUG
void
pq_print(struct pq * pq) {
    for (size_t i = 0; i < pq->size; i++) {
        struct pq_entry * entry = PQ_ENTRY(pq, i);
        struct pq_entry * parent = PQ_ENTRY(pq, PQ_PARENT(i));
        if (parent->priority > entry->priority) {
            printf("!");
        }
        printf("%lu ", entry->priority);
    }
    printf("\n");
}
#endif
