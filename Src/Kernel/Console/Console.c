#include <TextOS/TextOS.h>
#include <TextOS/Console.h>
#include <TextOS/Graphics.h>

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
}

void ConsoleClear ()
{
    Console.CurX = 0;
    Console.CurY = 0;
    
    ScreenClear();
}
