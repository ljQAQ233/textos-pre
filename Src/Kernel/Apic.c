#include <IO.h>

#define MPIC_DATA 0x21
#define SPIC_DATA 0xA1

void PicDisable ()
{
    OutB (SPIC_DATA, 0xFF);
    OutB (MPIC_DATA, 0xFF);
}

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

#define LAPIC_PA 0xFEE00000ULL
#define LAPIC_VA 0xFFFF80000A000000ULL

#define PUT()

void *LApic = (void *)LAPIC_PA;

#include <TextOS/Memory/Map.h>

void __Apic_SwitchMode ()
{
    VMMap (LAPIC_PA, LAPIC_VA, 4, PE_RW |PE_P, MAP_4K);

    LApic = (void *)LAPIC_VA;
}

void InitializeApic ()
{
    PicDisable(); // 这是废话,大家不要理它...
}