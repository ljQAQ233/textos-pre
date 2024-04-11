#include <TextOS/TextOS.h>
#include <TextOS/Args.h>
#include <TextOS/Dev.h>

#include <TextOS/Lib/VSPrint.h>

#define PRINTK_BUFFER_MAX 256

size_t PrintK (const char *Format, ...)
{
    va_list Args;
    va_start (Args, Format);

    char Buffer[PRINTK_BUFFER_MAX] = {0};

    size_t i = VSPrint (Buffer, Format, Args);

    Dev_t *Serial = DevLookupByType (DEV_CHAR, DEV_SERIAL);
    Dev_t *Console = DevLookupByType (DEV_CHAR, DEV_KNCON);

    Serial->Write (Serial, Buffer, -1);
    Console->Write (Console, Buffer, -1);

    va_end (Args);

    return i;
}

