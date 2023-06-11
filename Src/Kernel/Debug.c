#include <TextOS/Args.h>
#include <TextOS/Lib/VSPrint.h>
#include <TextOS/Console/PrintK.h>

static char Buffer[128];

void DebugK (
        const char *File,
        const u64  Line,
        const char *Format,
        ...
        )
{
    va_list Args;
    va_start (Args, Format);

    VSPrint (
            Buffer,
            Format,
            Args
        );

    PrintK ("[%s:%d] %s",File,Line,Buffer);

    va_end (Args);
}

