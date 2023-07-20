#ifndef __IRQ_H__
#define __IRQ_H__

#define INT_LAPIC_ERR 129 // Error handler
#define INT_LAPIC_SPS 130 // Spurious handler

#define IRQ_KEYBOARD 1
#define INT_KEYBOARD 0x21

bool IntrStateGet ();

void IntrStateEnable ();

void IntrStateDisable ();

/* 接下来是 APIC 的舞台! */
void LApic_SendEOI ();

#define _IOAPIC_RTE(Vector) ((u64)((u8)Vector))

void IOApicRteSet (u8 Irq, u64 Rte);

#endif
