#include <TextOS/TextOS.h>

#include <TextOS/Console.h>
#include <TextOS/Dev/Serial.h>
#include <TextOS/Debug.h>

extern void InitializeGdt ();
extern void InitializeIdt ();
extern void MemoryInit ();

#include <TextOS/Memory.h>
#include <TextOS/Memory/Map.h>

void KernelMain ()
{
    ConsoleInit();
    SerialInit();

    InitializeGdt();
    InitializeIdt();

    MemoryInit();

    char *ptr[5];
    ptr[0] = MallocK(2);
    ptr[1] = MallocK(74);
    ptr[2] = MallocK(25);
    ptr[3] = MallocK(2333333);

    FreeK (ptr[0]);
    FreeK (ptr[1]);
    FreeK (ptr[2]);
    FreeK (ptr[3]);
}
