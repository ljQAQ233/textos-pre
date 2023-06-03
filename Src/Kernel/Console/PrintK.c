#include <TextOS/TextOS.h>
#include <TextOS/Args.h>
#include <TextOS/Console.h>

#define BUFFER_MAX 32

static char Letter[] = "0123456789abcdef0123456789ABCDEF";

#define PRINT_BUFFER_MAX 256

static int PrintBufferIdx;
static char PrintBuffer[256];

static void __PrintReset ()
{
    PrintBufferIdx = 0;
}

static void __PrintSaveChar (char Char)
{
    PrintBuffer[PrintBufferIdx++] = Char;
}

static void __PrintSave (char *String)
{
    while (String && *String && PrintBufferIdx < PRINT_BUFFER_MAX) {
        PrintBuffer[PrintBufferIdx++] = *String++;
    }
}

static void __PrintHex (u64 Num)
{
    char Buffer[BUFFER_MAX] = {0};
    int i = BUFFER_MAX - 1;
    while (Num != 0) {
        Buffer[--i] = Letter[Num % 16];
        Num /= 16;
    }
    __PrintSave (&Buffer[i]);
}

static void __PrintHexUpper (u64 Num)
{
    char Buffer[BUFFER_MAX] = {0};
    int i = BUFFER_MAX - 1;
    while (Num != 0) {
        Buffer[--i] = Letter[(Num % 16) + 16];
        Num /= 16;
    }

    __PrintSave (&Buffer[i]);
}

static void __PrintUint64 (u64 Num)
{
    char Buffer[BUFFER_MAX] = {0};
    int i = BUFFER_MAX - 1;
    while (Num != 0) {
        Buffer[--i] = Letter[Num % 10];
        Num /= 10;
    }
    __PrintSave (&Buffer[i]);
}

static void __PrintInt64 (int64 Num)
{
    bool Negative = Num < 0 ? true : false;
    Num = ABS(Num);

    char Buffer[BUFFER_MAX] = {0};
    int i = BUFFER_MAX - 1;
    while (Num != 0) {
        Buffer[--i] = Letter[Num % 10];
        Num /= 10;
    }
    if (Negative) {
        Buffer[--i] = '-';
    }

    __PrintSave (&Buffer[i]);
}

static void __PrintInt32 (int32 Num)
{
    bool Negative = Num < 0 ? true : false;
    Num = ABS(Num);

    char Buffer[BUFFER_MAX] = {0};
    int i = BUFFER_MAX - 1;
    while (Num != 0) {
        Buffer[--i] = Letter[Num % 10];
        Num /= 10;
    }
    if (Negative) {
        Buffer[--i] = '-';
    }

    __PrintSave (&Buffer[i]);
}

static int __PrintK_Internal (char *Format, va_list Args)
{
    __PrintReset();

    while (*Format) {
        if (*Format == '%') {
Parse:
            switch (*++Format)
            {
                case '%':
                    __PrintSaveChar ('%');
                    break;

                case 'x':
                    __PrintHex (va_arg (Args, u64));
                    Format++;
                    continue;

                case 'X':
                    __PrintHexUpper (va_arg (Args, u64));
                    Format++;
                    continue;

                case 'd':
                    if (*(Format - 1) == 'l'
                            && *(Format - 2) == 'l') { 
                        __PrintInt64 (va_arg (Args, int64));
                    } else {
                        __PrintInt32 (va_arg (Args, int32));
                    }
                    Format++;
                    continue;

                case 'u':
                    __PrintUint64 (va_arg (Args, u64));
                    Format++;
                    continue;
                case 'l':
                    goto Parse;
            }
        }
        __PrintSaveChar (*Format++);
    }

    int i = 0;
    while (PrintBuffer[i++]);

    return i;
}

int PrintK (char *Format, ...)
{
    va_list Args;
    va_start (Args, Format);
    
    int i = __PrintK_Internal (Format, Args);

    ConsoleWrite (PrintBuffer);

    va_end (Args);

    return i;
}

