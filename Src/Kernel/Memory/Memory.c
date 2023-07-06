#include <TextOS/Debug.h>

u64 MemoryTotal;
u64 PagesTotal;

extern void PMM_Init ();
extern void PMM_EarlyInit ();
extern void VMMap_Init ();
extern void VMMap_EarlyInit ();

extern void Heap_Init ();

void MemoryInit ()
{
    PMM_EarlyInit();  // 早期初始化
    VMMap_EarlyInit();

    Heap_Init();
    PMM_Init();       // 总不可能把它抛弃了吧...

    VMMap_Init();
}
