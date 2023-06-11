#include <TextOS/TextOS.h>

#include <TextOS/Console.h>
#include <TextOS/Assert.h>

extern void InitializeGdt ();

void KernelMain ()
{
    ConsoleInit();

    InitializeGdt();
}
