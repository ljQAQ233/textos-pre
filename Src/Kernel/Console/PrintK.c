#include <TextOS/TextOS.h>
#include <TextOS/Args.h>
#include <TextOS/Console.h>
#include <TextOS/Dev/Serial.h>
#include <TextOS/Lib/VSPrint.h>

#define PRINTK_BUFFER_MAX 256

size_t PrintK (const char *Format, ...)
{
    va_list Args;
    va_start (Args, Format);

    char Buffer[PRINTK_BUFFER_MAX] = {0};

    size_t i = VSPrint (Buffer, Format, Args);

    SerialWrite (Buffer);
    ConsoleWrite (Buffer);

    va_end (Args);

    return i;
}

