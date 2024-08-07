#ifndef __MEMORY_H__
#define __MEMORY_H__

/* allocate `num` physical pages */
void *pmm_allocpages (size_t num);

/* hardcore! */
void pmm_allochard (void *page, size_t num);

/* free physical pages */
void pmm_freepages (void *page, size_t num);

void *malloc (size_t siz);

void free (void *addr);

#include <textos/mm/map.h>

#endif
