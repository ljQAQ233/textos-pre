/* RTC driver */
#include <IO.h>

#define R_SECOND   0x00
#define R_MINUTE   0x02
#define R_HOUR     0x04
#define R_WEEKDAY  0x06
#define R_DAY      0x07
#define R_MONTH    0x08
#define R_YEAR     0x09
#define R_STAT_A   0x0A
#define R_STAT_B   0x0B
#define R_CENTURY  0x32 // maybe

#define STAT_NOBCD   (1 << 3)

#include <string.h>
#include <TextOS/Debug.h>

#define R_CMOS_ADDR 0x70 // the port to address register which will be written or read
#define R_CMOS_DATA 0x71 // the port to write or read data

static inline u8 ReadRtc (u16 Reg)
{
    /* Close the NMI with the flag 0x80 */
    OutB (R_CMOS_ADDR, Reg | (1 << 7));
    return InB (R_CMOS_DATA);
}

static u64 __StartupTime;

#define DUMP_BCD(Bcd) \
    ((Bcd >> 4) * 10 + (Bcd & 0xF))

#define CENTURY_DEFAULT (21)

#include <TextOS/Lib/Time.h>

void ClockInit ()
{
    Time_t Tm;
    memset (&Tm, 0, sizeof(Time_t));

    /* 让 CMOS 自己告诉我们它有没有用 BCD */
    bool BcdMode = !(ReadRtc (R_STAT_B) & STAT_NOBCD);

    /* 在获取时间时, 尽量把容易改变的放在最后 */
    u8 Century;
    if (false) { // TODO
        Century = ReadRtc (R_CENTURY);
        if (BcdMode)
          Century = DUMP_BCD(Century);
    } else {
        Century = CENTURY_DEFAULT;
    } 

    Tm.Year    = ReadRtc (R_YEAR);
    Tm.Month   = ReadRtc (R_MONTH);
    Tm.Day     = ReadRtc (R_DAY);
    Tm.Hour    = ReadRtc (R_HOUR);
    Tm.Minute  = ReadRtc (R_MINUTE);
    Tm.Second  = ReadRtc (R_SECOND);
    
    if (BcdMode)
    {
        Tm.Year    = DUMP_BCD(Tm.Year);
        Tm.Month   = DUMP_BCD(Tm.Month);
        Tm.Day     = DUMP_BCD(Tm.Day);
        Tm.Hour    = DUMP_BCD(Tm.Hour);
        Tm.Minute  = DUMP_BCD(Tm.Minute);
        Tm.Second  = DUMP_BCD(Tm.Second);
    }
    Tm.Year += 100 * (Century - 1);

    u64 Stamp = TimeStamp (&Tm);
    DEBUGK ("Time now -> %u/%u/%u %02u:%02u:%02u (%llu)",
           Tm.Year, Tm.Month, Tm.Day,
           Tm.Hour, Tm.Minute, Tm.Second, Stamp);
}

