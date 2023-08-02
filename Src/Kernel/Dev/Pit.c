#define R_CNT0 0x40
#define R_CNT1 0x41
#define R_CNT2 0x42
#define R_MCMD 0x43

#define CNT_0 0x0
#define CNT_1 0x1
#define CNT_2 0x2

#define CMD_PACK(Cnt, Acc, Opt, Bcd) \
     ((((u8)Cnt & 0b11) << 6) | (((u8)Acc & 0b11) << 4) \
    | (((u8)Opt & 0b111) << 1) | ((u8)Bcd & 0b1))

#define M_TERM  0b000
#define M_HWTRO 0b001
#define M_RATE  0b010
#define M_SQR   0b011

#define FREQ   1193182
#define IC_S   FREQ
#define IC_MS  (IC_S / 1000)

#include <IO.h>
#include <Irq.h>
#include <Intr.h>

static inline void _PitMs ()
{
    /*
       通道2 - 先写低位,再写高位 - Interrupt On Terminal Count - BCD disabled
    */
    OutB(R_MCMD, CMD_PACK(CNT_2, 0b11, M_TERM, false));

    OutB(R_CNT2, IC_MS & 0xFF);
    OutB(R_CNT2, (IC_MS >> 8) & 0xFF);

    /* 等待计时器 (Cnt 2) 计数结束
       0x61 为 NMI Status and Control Register 

       Timer Counter 2 OUT Status (TMR2_OUT_STS) — RO. This bit reflects the current 
       state of the 8254 counter 2 output.
    */
    while ((InB(0x61) & 0x20) == 0);
}

void PitSleep (int Ms)
{
    while (Ms--) {
        _PitMs();
    }
}
