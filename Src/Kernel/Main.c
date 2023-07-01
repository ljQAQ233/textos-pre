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

    VMMap (0x1000, 0x233333333000, 1, PE_P | PE_RW | 0x23, MAP_4K);
    char *vptr = (char *)0x233333333000;
    char *pptr = (char *)0x1000;

    DEBUGK ("%c\n", *vptr);
    *pptr = 'T';
    DEBUGK ("%c\n", *vptr);
}
