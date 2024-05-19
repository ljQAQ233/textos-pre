#include <TextOS/Time.h>

static u32 Month[13] = {
/*
    这个月以及之前包含的天数为 (Month - 1) , 从 0 开始, Why? 因为一月已经是第一个月了
    这个表只考虑到了平年, 闰年也就多了那一天
    1    2    3    4    5    6    7    8    9    10   11   12
*/
    0,
    31,
    31 + 28,
    31 + 28 + 31,
    31 + 28 + 31 + 30,
    31 + 28 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30,
    31 + 28 + 31 + 30 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31
};

#define LEAP_CKR(Year)  \
     (  Year % 100 == 0 \
      ? Year % 400 == 0 \
      : Year % 4   == 0 \
     )                  \

/* XX 的时间使用秒来表示 */
#define TS_MINUTE (60)
#define TS_HOUR   (60 * 60)
#define TS_DAY    (60 * 60 * 24)

u64 TimeStamp (Time_t *Tm)
{
    /*
       计算闰年润过来的天数
       这里 Y < 2000 时, "向上取整"的结果依然是 0
    */
    u64 Y = Tm->Year - 1;
    u64 Offset = DIV_ROUND_UP(Y - 1972, 4)
               - DIV_ROUND_UP(Y - 2000, 100)
               + DIV_ROUND_UP(Y - 2000, 400);
    /* 全是平年的话, 度过的天数 */
    u64 Common = (Tm->Year - 1970) * 365;

    /* 计算在今年这天之前度过了多少天 */
    u64 Days = Month[Tm->Month - 1] + (LEAP_CKR(Tm->Year) ? 1 : 0)
             + (Tm->Day - 1);
    
    /* 综上所述... */
    u64 Res = (Common + Offset + Days) * TS_DAY
            + Tm->Hour * TS_HOUR
            + Tm->Minute * TS_MINUTE
            + Tm->Second;
    return Res;
}

