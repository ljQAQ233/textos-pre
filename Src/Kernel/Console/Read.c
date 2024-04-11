#include <TextOS/Dev.h>
#include <TextOS/Console.h>

int ConsoleRead (Dev_t *Con, char *Buffer, size_t Count)
{
    Dev_t *Kbd = DevLookupByType (DEV_CHAR, DEV_KBD);

    return Kbd->Read (Kbd, Buffer, Count);
}

char CharGet ()
{
    Dev_t *Con = DevLookupByType (DEV_CHAR, DEV_KNCON);

    char Chr = 0;
    Con->Read (Con, &Chr, 1);
    if (Console.Echo)
        Con->Write (Con, &Chr, 1);

    return Chr;
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


