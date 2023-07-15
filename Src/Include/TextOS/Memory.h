#ifndef __MEMORY_H__
#define __MEMORY_H__

extern u64 MemoryTotal;
extern u64 PagesTotal;

void PMM_AllocHard (void *Page, size_t Num);

void *PMM_AllocPages (size_t Num);

void PMM_FreePages (void *Page, size_t Num);

void *MallocK (size_t Siz);

void FreeK (void *Addr);

#endif
