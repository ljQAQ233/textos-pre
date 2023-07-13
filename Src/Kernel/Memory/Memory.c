#include <TextOS/Debug.h>

u64 MemoryTotal;
u64 PagesTotal;

extern void PMM_Init ();

void MemoryInit ()
{
    PMM_Init();
}
