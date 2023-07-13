#include <TextOS/TextOS.h>
#include <TextOS/Uefi.h>

#include <Boot.h>

extern u64 MemoryTotal;
extern u64 PagesTotal;

void __MemoryInit (BOOT_CONFIG *Config)
{
    MAP_INFO *Info = Config->Memory.Map;
    void *Ptr = Info->Maps;

    PagesTotal = 0;
    for (size_t i = 0;i < Info->MapCount;i++, Ptr += Info->DescSiz) {
        PagesTotal += ((EFI_MEMORY_DESCRIPTOR *)Ptr)->NumberOfPages;
    }
    MemoryTotal = PagesTotal * PAGE_SIZ;
}
