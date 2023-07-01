#include <Cpu.h>
#include <string.h>
#include <TextOS/Debug.h>
#include <TextOS/Assert.h>
#include <TextOS/Memory.h>
#include <TextOS/Memory/Map.h>

static u64 *PML4;

/* Value */
#define PE_V_FLAGS(Entry) (((u64)Entry) & 0x7FF)
#define PE_V_ADDR(Entry) (((u64)Entry) & ~0x7FF)

/* Set */
#define PE_S_FLAGS(Flgs) (u64)(Flgs & 0x7FF)
#define PE_S_ADDR(Addr) (u64)(((u64)Addr & ~0x7FF))

#define IDX(Addr, Level) (((u64)Addr >> ((int)(Level - 1) * 9 + 12)) & 0x1FF)

enum Level {
    L_PML4 = 4,
    L_PDPT = 3,
    L_PD   = 2,
    L_PT   = 1,
    L_PG   = 0,
};

static void
_MapWalk (u64 Phy, u64 Vrt, u16 Flgs, u64 *Tab, int Level, int Mode)
{
    u64 Idx = IDX(Vrt, Level);

    if (Level == Mode) {
        Tab[Idx] = PE_S_ADDR(Phy) | PE_S_FLAGS(Flgs);
        return;
    }
    else if (!(Tab[Idx] & PE_P))
    {
        void *New = PMM_AllocPages(1);
        ASSERTK (New != NULL);

        memset (New, 0, PAGE_SIZ);
        Tab[Idx] = PE_S_ADDR(New) | PE_RW | PE_P;
    }
    Tab = (u64 *)PE_V_ADDR(Tab[Idx]);

    _MapWalk (Phy, Vrt, Flgs, Tab, --Level, Mode);
}

static inline u16
_SpecialFlgs (int Mode)
{
    if (Mode == MAP_2M)
        return PDE_2M;
    else if (Mode == MAP_1G)
        return PDPTE_1G;

    return 0;
}

static inline u32
_PageSiz (int Mode)
{
    if (Mode == MAP_2M) {
        return SIZE_2MB;
    } else if (Mode == PDPTE_1G) {
        return SIZE_1GB;
    }

    return SIZE_4KB;
}

void VMMap (u64 Phy, u64 Vrt, size_t Num, u16 Flgs, int Mode)
{
    DEBUGK ("Try to map %p -> %p - %llu,%x,%d\n", Phy, Vrt, Num, Flgs, Mode);

    u64 PageSiz = _PageSiz (Mode);
    while (Num--) {
        _MapWalk (Phy, Vrt, Flgs | _SpecialFlgs (Mode), PML4, L_PML4, Mode);
        Phy += PageSiz;
        Vrt += PageSiz;
    }

    __asm__ volatile ("invlpg (%0)" : : "r"(Vrt) : "memory");
}

void VMMapInit ()
{
    PML4 = (u64 *)PE_V_ADDR (ReadCr3());
}

