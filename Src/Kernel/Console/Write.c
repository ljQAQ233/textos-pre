#include <TextOS/TextOS.h>
#include <TextOS/Console.h>

static int ConsoleWriteChar (char Char)
{
    switch (Char)
    {
        case '\n': // 回车,将指针移动到 (0,CurY+1)
            Console.CurX = 0;
            Console.CurY++;
            return Char;
        case '\r': // 将指针移动到 (0,CurY)
            Console.CurX = 0;
            return Char;
        case '\b': // 将指针移动到 (CurX-1,CurY)
            Console.CurX--;
            return Char;
        case '\f': // 进纸符
            Console.CurY++;
            return Char;
    }

    if (Console.CurX + 1 == Console.Row) {
        Console.CurY++;
        Console.CurX = 0;
    }
    if (Console.CurY + 1 == Console.Col) {
        ConsoleClear();
    }

    u16 X = Console.CurX * Font->Width;
    u16 Y = Console.CurY * Font->Height;

    Console.CurX++;

    return FontShow (
        Char,
        Font,
        X,Y,Console.FGColor,Console.BGColor
        );
}

int ConsoleWrite (char *String)
{
    int Length = 0;
    
    while (*String && String)
    {
        Length++;
        ConsoleWriteChar (*String++);
    }

    return Length + 1;
}
