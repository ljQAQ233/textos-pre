#include <TextOS/TextOS.h>

#include <TextOS/Console.h>
#include <TextOS/Assert.h>

extern void InitializeGdt ();
extern void InitializeIdt ();

void KernelMain ()
{
    ConsoleInit();

    InitializeGdt();
    InitializeIdt();
}
