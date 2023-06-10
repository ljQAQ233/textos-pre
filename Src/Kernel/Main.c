#include <TextOS/TextOS.h>

#include <TextOS/Console.h>
#include <TextOS/Console/PrintK.h>

void KernelMain ()
{
    ConsoleInit();

    PrintK ("Test format : %#x\n",-2333U);
    PrintK ("Test format : %#lx\n",-2333UL);
    PrintK ("Test format : %llx\n2333",-2333U);
}

