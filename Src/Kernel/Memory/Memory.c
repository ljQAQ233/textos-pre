#include <TextOS/Debug.h>

u64 MemoryTotal;
u64 PagesTotal;

extern void PMM_Init ();
extern void VMMapInit ();

extern void HeapInit ();

void MemoryInit ()
{
    PMM_Init();
    VMMapInit();

    HeapInit();
}
