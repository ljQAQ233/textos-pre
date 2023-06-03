#include <TextOS/TextOS.h>

#include <TextOS/Console.h>
#include <TextOS/Console/PrintK.h>

void KernelMain ()
{
    ConsoleInit();

    PrintK ("Test format : %llx\n",-2333ULL);
}

