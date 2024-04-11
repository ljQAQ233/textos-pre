#include <TextOS/TextOS.h>
#include <TextOS/Console.h>
#include <TextOS/Graphics.h>

#include <TextOS/Dev.h>
#include <TextOS/Dev/Private.h>

extern int ConsoleRead (Dev_t *Con, char *Buffer, size_t Count);
extern size_t ConsoleWrite (Dev_t *Dev, char *String, size_t Count);

static DevPri_t Dev = {
    .Dev = &(Dev_t) {
        .Name = "Console (kernel builtin)",
        .Read  = (void *)ConsoleRead,
        .Write = (void *)ConsoleWrite,
        .Type  = DEV_CHAR,
        .SubType = DEV_KNCON,
    },
};

Console_t Console;

void ConsoleInit ()
{
    Console.Hor = Hor;
    Console.Ver = Ver;
    
    Console.CurX = 0;
    Console.CurY = 0;

    FontInfo_t *Font = FontGet(0);
    
    Console.Font = Font;
    
    Console.Row = Hor / Font->Width;
    Console.Col = Ver / Font->Height;

    Console.BGColor = 0x00000000;
    Console.FGColor = 0x00ffffff;

    Console.Echo = true; // 开启回显

    __DevRegister (&Dev);
}

void ConsoleClear ()
{
    Console.CurX = 0;
    Console.CurY = 0;
    
    ScreenClear();
}
