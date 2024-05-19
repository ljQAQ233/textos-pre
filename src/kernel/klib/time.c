#include <textos/klib/time.h>

static u32 month[13] = {
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

#define LEAP_CKR(year)  \
     (  year % 100 == 0 \
      ? year % 400 == 0 \
      : year % 4   == 0 \
     )                  \

/* XX 的时间使用秒来表示 */
#define TS_MINUTE (60)
#define TS_HOUR   (60 * 60)
#define TS_DAY    (60 * 60 * 24)

u64 time_stamp (time_t *tm)
{
    /*
       计算闰年润过来的天数
       这里 Y < 2000 时, "向上取整"的结果依然是 0
    */
    u64 Y = tm->year - 1;
    u64 offset = DIV_ROUND_UP(Y - 1972, 4)
               - DIV_ROUND_UP(Y - 2000, 100)
               + DIV_ROUND_UP(Y - 2000, 400);
    /* 全是平年的话, 度过的天数 */
    u64 common = (tm->year - 1970) * 365;

    /* 计算在今年这天之前度过了多少天 */
    u64 days = month[tm->month - 1] + (LEAP_CKR(tm->year) ? 1 : 0)
             + (tm->day - 1);
    
    /* 综上所述... */
    u64 res = (common + offset + days) * TS_DAY
            + tm->hour * TS_HOUR
            + tm->minute * TS_MINUTE
            + tm->second;
    return res;
}

