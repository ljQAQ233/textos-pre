#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <TextOS/Graphics/Font.h>

struct _CONSOLE
{
    u32 Hor;    /* HorizontalResolution */
    u32 Ver;    /* VerticalResolution   */
    u16 Row;    /* Row count       */
    u16 Col;    /* Column count    */
    u16 CurX;   /* Cursor address X */
    u16 CurY;   /* Cursor address Y */
    u32 FGColor;
    u32 BGColor;
    FontInfo_t *Font;
};

typedef struct _CONSOLE Console_t;

extern Console_t Console;

void ConsoleInit ();
void ConsoleClear ();

#include "Console/Write.h"

#endif