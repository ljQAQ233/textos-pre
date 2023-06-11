#include <TextOS/TextOS.h>

#include <TextOS/Console.h>
#include <TextOS/Assert.h>

void KernelMain ()
{
    ConsoleInit();

    ASSERTK (1 == 1);
    ASSERTK (1 == 0);
}

