#include <TextOS/TextOS.h>

#include <TextOS/Console.h>

void KernelMain ()
{
    ConsoleInit();
    
    ConsoleWrite ("Hello world!\n");
    ConsoleWrite ("Hello world!\rM");
    ConsoleWrite ("Hello world!\b?");
    ConsoleWrite ("Hello world!\f?");
}

