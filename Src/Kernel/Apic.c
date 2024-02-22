#include <IO.h>
#include <Cpu.h>
#include <Irq.h>
#include <Intr.h>
#include <TextOS/Dev/8259.h>

#include <TextOS/Panic.h>
#include <TextOS/Memory.h>
#include <TextOS/Memory/Map.h>

/* LApiC's registers */
#define LAPIC_ID   0x20
#define LAPIC_VER  0x30
#define LAPIC_TPR  0x80
#define LAPIC_APR  0x90
#define LAPIC_PPR  0xA0
#define LAPIC_EOI  0xB0
#define LAPIC_RRD  0xC0
#define LAPIC_LDR  0xD0   // Logical dest register
#define LAPIC_DFR  0xE0   // Destination fmt register
#define LAPIC_SVR  0xF0   // Spurious Interrupt Vector Register
#define LAPIC_ISR  0x100
#define LAPIC_TMR  0x180
#define LAPIC_IRR  0x200
#define LAPIC_ESR  0x280  // Error status register
#define LAPIC_CMCI 0x2F0  // LVT Corrected Machine Check Interrupt Register
#define LAPIC_ICR  0x300
#define LAPIC_TM   0x320  // LVT timer register
#define LAPIC_TRML 0x330  // TheRMaL sensor register
#define LAPIC_PC   0x340  // Performance counter
#define LAPIC_INT0 0x350
#define LAPIC_INT1 0x360
#define LAPIC_ERR  0x370  // LVT Error Register
#define LAPIC_TICR 0x380  // Initial Counter Register for timer
#define LAPIC_TCCR 0x390  // Current Counter Register for timer
#define LAPIC_DCR  0x3E0  // Divide Configuration Register for timer

#define LVT_MASK (1 << 16)

#define LAPIC_VA  0xFFFFFF0000008000ULL
#define IOAPIC_VA 0xFFFFFF0000000000ULL

void *LApic = NULL;
void *IOApic = NULL;

#define LAPIC_GET(Reg)         (*((u32 volatile *)(LApic + Reg)))
#define LAPIC_SET(Reg, Val)    (*((u32 volatile *)(LApic + Reg)) = (u32)(Val))

#define S_TPR(TC, TSC)         ((u32)TC << 4 | (u32)TSC)
#define S_SVR(Stat, Vector)    ((((u32)Stat & 1) << 8) | ((u32)Vector & 0xFF))

#define S_TM(Mode, Vector) ((((u32)Mode & 0b11) << 17 | (u8)Vector))

#define TM_ONESHOT  0b00 // One-Shot Mode
#define TM_PERIODIC 0b01 // Periodic Mode 周期模式
#define TM_TSCDLN   0b10 // TSC-Deadline Mode

void __Apic_SwitchMode ()
{
    VMMap ((u64)LApic, LAPIC_VA, 1, PE_RW | PE_P, MAP_4K);
    VMMap ((u64)IOApic, IOAPIC_VA, 1, PE_RW | PE_P, MAP_4K);

    LApic = (void *)LAPIC_VA;
    IOApic = (void *)IOAPIC_VA;
}

void LApicErrHandler () { PANIC ("Apic Handler is not supported!!!"); }
void LApicSpuriousHandler () { ; }


__INTR_FUNC(TimerHandler)
{
    LApic_SendEOI();
}

#include <TextOS/Dev/Pit.h>

/* Apic 不可以启动再禁用后再启动, 除非重启机器 */
void InitializeApic ()
{
    if (LApic == NULL || IOApic == NULL)
        PANIC ("Invalid LApic or IOApic. Is not detected\n");

    PicDisable(); // 某种意义上,这是废话,大家不要理它...

    WriteMsr (IA32_APIC_BASE, ((u64)LApic << 12) | (1 << 11));

    IntrRegister (INT_LAPIC_ERR, (IntrCaller_t)LApicErrHandler);
    IntrRegister (INT_LAPIC_SPS, (IntrCaller_t)LApicSpuriousHandler);

    LAPIC_SET(LAPIC_ERR, 129);

    LAPIC_SET(LAPIC_ESR, 0);
    LAPIC_SET(LAPIC_ESR, 0);

    LAPIC_SET(LAPIC_TPR, S_TPR(0, 0));
    LAPIC_SET(LAPIC_SVR, S_SVR(true, INT_LAPIC_SPS));  // 软启用 Apic / APIC Software Enable

    LAPIC_SET(LAPIC_TM, S_TM(TM_ONESHOT, 0) | LVT_MASK); // 屏蔽 Apic Timer 的本地中断
    LAPIC_SET(LAPIC_DCR, 0b0000);                        // 设置 除数 (因子) 为 2
    LAPIC_SET(LAPIC_TICR, 0xFFFFFFFF);                   // 设置 初始计数到最大 (-1)

    PitSleep (10);

    LAPIC_SET(LAPIC_TICR, 0xFFFFFFFF - LAPIC_GET(LAPIC_TCCR)); // 计算 100ms 的 ticks
    LAPIC_SET(LAPIC_TM, S_TM(TM_PERIODIC, INT_TIMER));         // 步入正轨, 每 100ms 产生一次时钟中断
    IntrRegister (INT_TIMER, TimerHandler);                    // 注册中断函数
}

void LApic_SendEOI ()
{
    LAPIC_SET (LAPIC_EOI, 0);
}

extern u8 __GsiGet (u8 Src);

void IOApicRteSet (u8 Irq, u64 Rte)
{
    Irq = __GsiGet (Irq);  // 向 Acpi 查询是否存在 Src -> Gsi

    u32 *RegSel = (u32 *)(IOApic);
    u32 *Data   = (u32 *)(IOApic + 0x10);

    *RegSel = Irq * 2 + 0x10;
    *Data = Rte & 0xFFFFFFFF;

    *RegSel = Irq * 2 + 0x10 + 1;
    *Data = Rte >> 32;
}

