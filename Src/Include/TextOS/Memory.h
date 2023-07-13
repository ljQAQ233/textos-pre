#ifndef __MEMORY_H__
#define __MEMORY_H__

extern u64 MemoryTotal;
extern u64 PagesTotal;

void *PMM_AllocPages (size_t Num);

void PMM_FreePages (void *Page, size_t Num);

#endif
