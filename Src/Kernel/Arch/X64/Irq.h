#ifndef __IRQ_H__
#define __IRQ_H__

#define INT_LAPIC_ERR 129 // Error handler
#define INT_LAPIC_SPS 130 // Spurious handler

#define IRQ_TIMER 0
#define INT_TIMER 0x20

#define IRQ_KEYBOARD 1
#define INT_KEYBOARD 0x21

#define IRQ_MDISK 14
#define INT_MDISK 0x22

bool IntrStateGet ();

void IntrStateEnable ();

void IntrStateDisable ();

#define UNINTR_AREA_START()                      \
        bool __IntrStat__ = IntrStateGet();      \
        IntrStateDisable();                      \

#define UNINTR_AREA_END()                        \
        if (__IntrStat__) IntrStateEnable();     \

#define UNINTR_AREA(Opts)                            \
        do {                                         \
            UNINTR_AREA_START();                     \
            Opts;                                    \
            UNINTR_AREA_END();                       \
        } while (false);                             \

/* 接下来是 APIC 的舞台! */
void LApic_SendEOI ();

#define _IOAPIC_RTE(Vector) ((u64)((u8)Vector))

void IOApicRteSet (u8 Irq, u64 Rte);

#endif
