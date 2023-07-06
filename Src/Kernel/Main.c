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
    
    DEBUGK ("Test graphic module\n");
}
