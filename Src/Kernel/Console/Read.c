#include <TextOS/Dev.h>
#include <TextOS/Console.h>

int ConsoleRead (char *Buffer, size_t Count)
{
    Dev_t *Kbd = DevLookupByType (DEV_CHAR, DEV_KBD);

    int i = Kbd->Read (Buffer, Count);
    Buffer[i] = EOS;

    return i;
}

char CharGet ()
{
    char Chr[2] = "";

    ConsoleRead (Chr, 1);
    if (Console.Echo)
        ConsoleWrite (Chr);

    return Chr[0];
}

char *LineGet (char *Buffer)
{
    size_t i = 0;
    while ((Buffer[i] = CharGet()) != EOL) i++;

    Buffer[i] = EOS;

    return Buffer;
}

char *StringGet (char *Buffer, size_t Cnt)
{
    size_t i = 0;
    while (i < Cnt)
        Buffer[i++] = CharGet();

    Buffer[i] = EOS;

    return Buffer;
}


