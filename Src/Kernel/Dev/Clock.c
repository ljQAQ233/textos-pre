#include <IO.h>
#include <Irq.h>
#include <TextOS/Debug.h>

#define R_CMOS_REG  0x70
#define R_CMOS_DATA 0x71

#define RTC_SEC     0x00
#define RTC_MIN     0x02
#define RTC_HOUR    0x04
#define RTC_WEEKDAY 0x06
#define RTC_DAY     0x07
#define RTC_MONTH   0x08
#define RTC_YEAR    0x09
#define RTC_STAT_A  0x0A
#define RTC_STAT_B  0x0B

#define SA_H24    (1 << 2) // 24-hour format
#define SA_BINARY (1 << 2) // Binary mode enable. If 0, use BCD format

#define FREQ 32768

static u8 ReadCMos (u8 Idx)
{
    Idx |= (1 << 7);

    OutB (R_CMOS_REG, Idx);
    return InB (R_CMOS_DATA);
}

static void WriteCMos (u8 Idx, u8 Data)
{
    Idx |= (1 << 7);

    OutB (R_CMOS_REG, Idx);
    OutB (R_CMOS_DATA, Data);
}

static void NmiEnable ()
{
    OutB (R_CMOS_REG, 0);
}

/* Bcd code uses 4-bit to describe a number like "0b00110010 -> 32" */
static inline u8 _ConvertBcd (u8 Bcd)
{
    return (Bcd >> 4) * 10 + (Bcd & 0xF);
}

#include <TextOS/Acpi.h>

void ClockInit ()
{
    /* Not disable NMI here because we haven't enable it... */

    u8 StatB = ReadCMos (RTC_STAT_B);

    u8 Secnod = ReadCMos (RTC_SEC);
    u8 Minute = ReadCMos (RTC_MIN);
    u8 Hour   = ReadCMos (RTC_HOUR);
    u8 Day    = ReadCMos (RTC_DAY);
    u8 Month  = ReadCMos (RTC_MONTH);
    u16 Year   = ReadCMos (RTC_YEAR);

    u8 Century = 0b00100000;
    u8 Idx = __Acpi_CenturyField();
    if (Idx != 0)
        Century = ReadCMos (Idx);
    else {
        if (Year >= 90) {
            /* Well, we guess it is 20 century...
               Use BCD code here                  */
            Century = 0b00011001;
        }
    }

    if (!(StatB & SA_BINARY)) {
        Secnod  = _ConvertBcd (Secnod);
        Minute  = _ConvertBcd (Minute);
        Hour    = _ConvertBcd (Hour);
        Day     = _ConvertBcd (Day);
        Month   = _ConvertBcd (Month);
        Year    = _ConvertBcd (Year);
        Century = _ConvertBcd (Century);
    }
    Year += Century * 100; // A bit strange...

    DEBUGK ("%u/%u/%u %u:%u:%u\n", Year, Month, Day, Hour, Minute, Secnod);
}
