#include <TextOS/TextOS.h>
#include <TextOS/Args.h>

#include <Irq.h>

#define TMP_BUFFER_SIZE 64

enum {
  LEFT    = 1,
  SIGN    = 1 << 1,
  ZERO    = 1 << 2,
  SPECIAL = 1 << 3,
  SPACE   = 1 << 4,
};

#define IsDigit(Object) ('0' <= (char)Object && (char)Object <= '9')

static int _Int (char *Ptr, int *Width)
{
    int i = 0;
    int l = 0;

    while (IsDigit (*Ptr)) {
        i = i * 10 + *Ptr++ - '0';
        l++;
    }

    *Width = i;

    return l;
}

static int64 _Number (char *Buffer, u64 Num, int Base, bool Upper)
{
    char *Letters = !Upper ? "0123456789abcdef" : "0123456789ABCDEF";

    int64 Siz = 0;
    char Tmp[TMP_BUFFER_SIZE];

    char *Ptr = Tmp;

    if (Num == 0) {
        *Ptr++ = '0';
        Siz++;
    } else {
        while (Num != 0) {
            *Ptr++ = Letters[Num % Base];
            Num /= Base;
            Siz++;
        }
    }

    Ptr--;
    int64 i = 0;
    while (i < Siz) {
        Buffer[i++] = *Ptr--;
    }
    Buffer[i] = '\0';

    return Siz;
}

int64 VSPrint (char *Buffer, const char *Format, va_list Args)
{
    UNINTR_AREA_START();

    char *Out = Buffer;
    char *Ptr = (char*)Format;

    int Flgs = 0;

    while (Ptr && *Ptr)
    {
        if (*Ptr != '%')
        {
            *Out++ = *Ptr++;
            continue;
        }
ParseFlgs:
        Ptr++;
        switch (*Ptr) {
            case '#': // 与 o,x或X 一起使用时,非零值前面会分别显示 0,0x或0X
                Flgs |= SPECIAL;
                goto ParseFlgs;
            case '0': // 在指定填充的数字左边放置0,而不是空格
                if (Flgs & ZERO) {
                    break; // 再有就是宽度
                }
                Flgs |= ZERO;
                goto ParseFlgs;
            case '-': // 在给定的字段宽度内左对齐,默认是右对齐
                Flgs |= LEFT;
                goto ParseFlgs;
            case ' ': // 如果没有写入任何符号,则在该值前面填空格
                Flgs |= SPACE;
                goto ParseFlgs;
            case '+': // 如果是正数,则在最前面加一个正号
                Flgs |= SIGN;
                goto ParseFlgs;
            default:
                break;
        }
        Ptr--;

        int Offset = 0;

        int Radix = 10;
        int Length = 0;
        bool Signed = false;
        int Width = 0;
        bool UpperCase = false;
ParseArgs:
        Ptr++;
        switch (*Ptr)
        {
            case '%':
                *Out++ = '%';
                break;
            case 'l':
            case 'L':
                Length = 1;
                if (*(Ptr+1) == 'l' || *(Ptr+1) == 'L') {
                    Ptr++;
                    Length = 2;
                }
                goto ParseArgs;

            case 'X':
                UpperCase = true;
            case 'x':
                Radix = 16;
                break;
            case 'o':
                Radix = 8;
                break;
            case 'd':
            case 'i':
                Signed = true;
            case 'u':
                Radix = 10;
                break;
            case 'c':
                /* Includes the char */
                if (Width > 1)
                    while (--Width)
                        *Out++ = ' ';
                *Out++ = (char)va_arg (Args, int);

                Ptr++;
                continue;
            case 's':
                {
                    char *Src = (char *)va_arg (Args, char *);
                    if (Src == NULL)
                        Src = "(null)"; // 如果是 NULL 就填 "(null), 总不可能去访问 0x00 吧 QaQ"
                    for (char *p = Src;p && *p;p++)
                        Width--;
                    while (Width-- > 0)
                        *Out++ = ' ';
                    while (*Src)
                        *Out++ = *Src++;

                    Ptr++;
                    continue;
                }
            case 'p':
                Radix = 16;
                Length = 2;
                Flgs |= SPECIAL;
                break;
            case 'q':
                {
                    char Fill = (char)va_arg (Args, int);
                    for (int i = 0 ; i < Width ; i++) *Out++ = Fill;
                }

                Ptr++;
                continue;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '0':
                Offset = _Int (Ptr, &Width);
                Ptr += Offset - 1;
                goto ParseArgs;
            case '*':
                Width = va_arg (Args ,int);
                if (Width < 0)
                    Flgs |= LEFT; // 左对齐
                goto ParseArgs;
        }

        u64 Value;
        bool Minus = false;

        if (Length == 0)
        {
            Value = va_arg (Args, unsigned int);
            if (Signed && (int)Value < 0) {
                Minus = true;
                Value = -(int)Value; // 符号位将在最后于字符串上添上.
            }
        }
        else if (Length == 1)
        {
            Value = va_arg (Args, unsigned long);
            if (Signed && (long)Value < 0) {
                Minus = true;
                Value = -(long)Value;
            }
        }
        else if (Length == 2)
        {
            Value = va_arg (Args, unsigned long long);
            if (Signed && (long long)Value < 0) {
                Minus = true;
                Value = -(long long)Value;
            }
        }

        /* 每一次添加字符('+','-',' '...)都会导致 Siz 减小,
           这么做在于最后可以直接使用 Siz 来进行填充操作. */
        int64 Siz = Width;
        char Tmp[TMP_BUFFER_SIZE];
        Siz -= _Number (Tmp, Value, Radix, UpperCase);

        char Prefix = 0;
        if (Radix == 10)
        {
            if (Minus && Siz--)
                Prefix = '-';
            /* 以下是正数的情况 */
            else if (Signed && Flgs & SIGN && Siz--)
                Prefix = '+';
            else if (Flgs & SPACE && Siz--)
                Prefix = ' ';
        }

        if (Flgs & SPECIAL) {
            /* Prefix `0x` for hex or `0` for octal */
            Siz -= (Radix == 16) ? 2 :
                   (Radix == 8 ) ? 1 : 0;
        }

        if (Flgs & ZERO)
        {
            if (Prefix) *Out++ = Prefix;
            /* "0x" "0X" "0" for SPECIAL */
            if (Flgs & SPECIAL && Radix != 10)
            {
                *Out++ = '0';
                if (Radix == 16) {
                    *Out++ = UpperCase ? 'X' : 'x';
                }
            }
        }

        /* Padding */
        if (!(Flgs & LEFT) && Siz > 0)
        {
            char Pad = Flgs & ZERO ? '0' : ' ';

            while (Siz--) {
                *Out++ = Pad;
            }
        }

        /* Symbol and others after padding if that is not filled with '0' -> "    0x91d" */
        if (!(Flgs & ZERO))
        {
            if (Prefix) *Out++ = Prefix;
            /* "0x" "0X" "0" for SPECIAL */
            if (Flgs & SPECIAL && Radix != 10)
            {
                *Out++ = '0';
                if (Radix == 16) {
                    *Out++ = UpperCase ? 'X' : 'x';
                }
            }
        }

        for (char *p = Tmp;*p;) {
            *Out++ = *p++;
        }

        Ptr++;
    }
    *Out = '\0';
    
    UNINTR_AREA_END();

    return (int64)(Out - Buffer);
}

