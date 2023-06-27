#include <TextOS/TextOS.h>

#include <TextOS/Console.h>
#include <TextOS/Assert.h>

#include <TextOS/Dev/Serial.h>

extern void InitializeGdt ();
extern void InitializeIdt ();

void KernelMain ()
{
    ConsoleInit();
    SerialInit();

    SerialWrite ("Hello wolrd\n");

    InitializeGdt();
    InitializeIdt();
}
