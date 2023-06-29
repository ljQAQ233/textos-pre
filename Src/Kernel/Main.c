#include <TextOS/TextOS.h>

#include <TextOS/Console.h>
#include <TextOS/Dev/Serial.h>

extern void InitializeGdt ();
extern void InitializeIdt ();

#include <TextOS/Panic.h>

void KernelMain ()
{
    ConsoleInit();
    SerialInit();

    PANIC ("Panic test\n");

    InitializeGdt();
    InitializeIdt();
}
