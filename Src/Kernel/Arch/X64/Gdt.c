#include <Cpu.h>
#include <Gdt.h>
#include <string.h>
#include <TextOS/Assert.h>

typedef struct _packed
{
    u16 Limit; // gdt占用
    u64 Base;  // gdt的地址
} Gdtr_t;

typedef struct _packed
{
    u16 LimitLow  : 16;
    u32 BaseLow   : 24;
    union {
        struct {
            u8 A       : 1;
            u8 RW      : 1;
            u8 DC      : 1;
            u8 EXE     : 1;
            u8 S       : 1;
            u8 DPL     : 2;
            u8 Present : 1;
        };
        u8 Raw : 8;
    } AccessByte;
    u8  LimitHgh  : 4 ;
    u8  Flgs      : 4;
    u8  BaseHgh   : 8 ;
} Gdt_t;

#define GDT_MAX 16 // 16 个绝对够你用啦! /doge

#define F_L   (1 << 1) // Long mode
#define F_DB  (1 << 2)
#define F_G   (1 << 3)

#define A_A          (1)
#define A_RW         (1 << 1)
#define A_DC         (1 << 2)
#define A_EXE        (1 << 3)
#define A_NS         (1 << 4) // isn't system segment
#define A_P          (1 << 7)
#define A_DPL(DPL)   ((DPL & 0x3) << 5)

Gdt_t Gdts[GDT_MAX];
Gdtr_t Gdtr;

static void _GdtSetEntry (size_t i, u64 Base, u32 Limit, u8 AccessByte, u8 Flgs)
{
    Gdt_t *Ptr = &Gdts[i];

    Ptr->BaseLow = Base & 0xffffff;
    Ptr->BaseHgh = Base >> 24 & 0xff;
    Ptr->LimitLow = Limit & 0xffff;
    Ptr->LimitHgh = Limit >> 16 & 0xff;

    Ptr->Flgs = Flgs;
    Ptr->AccessByte.Raw = AccessByte;
}

#define KERN_CODE_A (A_P | A_DPL(0) | A_RW | A_EXE | A_NS)
#define KERN_DATA_A (A_P | A_DPL(0) | A_RW | A_NS)

#define USER_CODE_A (A_P | A_DPL(3) | A_RW | A_EXE | A_NS)
#define USER_DATA_A (A_P | A_DPL(3) | A_RW | A_NS)

#define CODE_F (F_L | F_G)
#define DATA_F (F_DB | F_G)

void InitializeGdt ()
{
    ASSERTK (sizeof(Gdt_t) == 8);

    memset (Gdts, 0, GDT_MAX * sizeof (Gdt_t));

    /* Keep the firt being 0 */

    _GdtSetEntry (KERN_CODE_SEG, 0, 0xFFFFF, KERN_CODE_A, CODE_F);
    _GdtSetEntry (KERN_DATA_SEG, 0, 0xFFFFF, KERN_DATA_A, DATA_F);
    _GdtSetEntry (USER_CODE_SEG, 0, 0xFFFFF, USER_CODE_A, CODE_F);
    _GdtSetEntry (USER_DATA_SEG, 0, 0xFFFFF, USER_DATA_A, DATA_F);

    Gdtr.Base = (u64)&Gdts;
    Gdtr.Limit = GDT_MAX * sizeof(Gdt_t);

    LoadGdt (&Gdtr);
    ReloadSegs (KERN_DATA_SEG, KERN_CODE_SEG);
}
