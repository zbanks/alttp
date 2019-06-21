#ifndef __PQ_H__
#define __PQ_H__

#include <stddef.h>
#include <stdint.h>

// Priority Queue (min-heap)

struct pq;

struct pq * pq_create(size_t data_size);
void pq_destroy(struct pq * pq);

void pq_clear(struct pq * pq);
int pq_push(struct pq * pq, uint64_t priority, const void * data);
int pq_pop(struct pq * pq, uint64_t * priority_out, void * data_out);
size_t pq_size(struct pq * pq);

#ifdef PQ_DEBUG
void pq_print(struct pq * pq);
#endif

#endif

