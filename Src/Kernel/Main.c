#include <TextOS/TextOS.h>

#include <TextOS/Console.h>
#include <TextOS/Dev/Serial.h>

extern void InitializeGdt ();
extern void InitializeIdt ();
extern void MemoryInit ();

#include <TextOS/Memory.h>

void KernelMain ()
{
    ConsoleInit();
    SerialInit();

    InitializeGdt();
    InitializeIdt();

    MemoryInit();

    void *Page;

    Page = PMM_AllocPages (1);
    PMM_FreePages(Page, 1);
    Page = PMM_AllocPages (5);
    PMM_FreePages(Page, 6);
}
