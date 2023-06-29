#include <Cpu.h>

#include <TextOS/TextOS.h>
#include <TextOS/Args.h>
#include <TextOS/Lib/VSPrint.h>

#include <string.h>
#include <TextOS/Debug.h>

static char Buffer[128];

void Panic (
        const char *File,
        const u64  Line,
        const char *Format,
        ...
        )
{
    va_list Args;
    va_start (Args, Format);

    VSPrint (Buffer, Format, Args);
    DebugK (File, Line, "Panic!!! -> %s",Buffer);

    va_end (Args);

    while (true) Halt();
}
