#include <TextOS/Args.h>
#include <TextOS/Lib/VSPrint.h>
#include <TextOS/Console/PrintK.h>

#include <string.h>

static char Buffer[128];

#ifdef __DEBUGK_PREETY
int _HeaderSiz (const char *Str, int Num)
{
    int Siz = 0;
    if (Num == 0) {
        Siz++;
    } else {
        while (Num != 0) {
            Num /= 10;
            Siz++;
        }
    }

    return Siz + strlen(Str) + 4;
}
#endif

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

#ifndef __DEBUGK_PREETY
    PrintK ("[%s:%d] %s",File,Line,Buffer);
#else
    char *Ptr = Buffer;
    int Sec = 0;

    while (Ptr && *Ptr) {
        if (*Ptr == '\n') {
            *Ptr = '\0';
            Sec++;
        }
        Ptr++;
    }

    int HeaderSiz = _HeaderSiz (File, Line);

    Ptr = Buffer;
    PrintK ("[%s:%d] %s\n",File,Line,Ptr);

    while (--Sec > 0) {
        for (int i = HeaderSiz; i ;i--) PrintK (" ");
        while (*Ptr++ != '\0') ;
        PrintK ("%s\n",Ptr);
    }
#endif

    va_end (Args);
}

