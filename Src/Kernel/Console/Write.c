#include <TextOS/TextOS.h>
#include <TextOS/Console.h>

static int ConsoleWriteChar (char Char)
{
    switch (Char)
    {
        case '\n': // 回车,将指针移动到 (0,CurY+1)
            Console.CurX = 0;
        case '\f': // 进纸符
            if (++Console.CurY >= Console.Col) {
                ConsoleClear();
            }
            return Char;
        case '\r': // 将指针移动到 (0,CurY)
            Console.CurX = 0;
            return Char;
        case '\b': // 将指针移动到 (CurX-1,CurY)
            Console.CurX = MAX (Console.CurX - 1, 0);
            return Char;
    }

    u16 X = Console.CurX * Console.Font->Width;
    u16 Y = Console.CurY * Console.Font->Height;

    FontShow (
        Char,
        Console.Font,
        X,Y,Console.FGColor, Console.BGColor
        );
    
    if (++Console.CurX >= Console.Row) {
        if (++Console.CurY >= Console.Col) {
            ConsoleClear();
        }
        Console.CurX = 0;
    }

    return Char;
}

#include <Irq.h>

size_t ConsoleWrite (char *String)
{
    char *Ptr;

    UNINTR_AREA ({
        for (Ptr = String ; Ptr && *Ptr ; Ptr++)
            ConsoleWriteChar (*Ptr);
    });

    return (size_t)(Ptr - String);
}

