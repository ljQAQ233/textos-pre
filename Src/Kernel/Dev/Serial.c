#include <IO.h>

#define COM1 0x3f8

//      Name    Offset  DLAB
#define R_DATA  0     // 0 Receive / Transfer
#define R_INTR  1     // 0 Interrupt Enable Register
#define R_LSB   0     // 1 Divisor Baud
#define R_MSB   1     // 1 
#define R_FIFO  2     // - Interrupt Identification / FIFO Control Register
#define R_LCR   3     // - The most significant bit of this register is the DLAB.
#define R_MCR   4     // - Modem Control Register
#define R_LSR   5     // - Line Status Register
#define R_MSR   6     // - Modem Status Register
#define R_SCR   7     // - Scratch Register

void SerialInit ()
{
    OutB (COM1 + R_INTR , 0);          // Disable interrupts
    OutB (COM1 + R_LCR  , 1 << 7);     // Enable DLAB
    OutB (COM1 + R_LSB  , 1);          // Set baud : 115200
    OutB (COM1 + R_MSB  , 0);          // High
    OutB (COM1 + R_LCR  , 0b11);       // 7 bits for data and 1 stop bit
    OutB (COM1 + R_FIFO , 0b11000001); // Enable FIFO and let trigger level 14

    OutB (COM1 + R_MCR  , 0b11110);    // 开启环回,检测开始

    OutB (COM1 + R_DATA , 0xae);
    if (InB (COM1 + R_DATA) != 0xae)
    {
        return;
    }

    OutB (COM1 + R_MCR, 0b100);        // 恢复 -> Out 1
}

static inline char _SerialRead ()
{
    while ((InB (COM1 + R_LSR) & 0x01) == 0);

    return InB (COM1 + R_DATA);
}

size_t SerialRead (char *Str, size_t Siz)
{
    for (size_t i = 0 ; i < Siz ; i++)
        *Str++ = _SerialRead (); 

    return Siz;
}

static inline void _SerialWrite (char Char)
{
    while ((InB (COM1 + R_LSR) & 0x20) == 0);

    OutB (COM1 + R_DATA, Char);
}

size_t SerialWrite (char *Str)
{
    char *Ptr = Str;
    while (Ptr && *Ptr)
        _SerialWrite (*Ptr++);

    return (size_t)(Ptr - Str);
}

