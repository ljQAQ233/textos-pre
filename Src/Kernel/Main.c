#include <TextOS/TextOS.h>

#include <TextOS/Console.h>
#include <TextOS/Debug.h>

void KernelMain ()
{
    ConsoleInit();

    DEBUGK ("Hello world : %p\n",&KernelMain);
}

