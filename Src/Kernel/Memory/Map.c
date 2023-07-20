#include <Cpu.h>
#include <string.h>
#include <TextOS/Debug.h>
#include <TextOS/Panic.h>
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

#define R_IDX 511ULL

#define VRT(iPML4, iPDPT, iPD, iPT) \
    ((u64)(((u64)iPML4 > 255 ? 0xFFFFULL : 0) << 48 \
    | ((u64)iPML4 & 0x1FF) << 39 \
    | ((u64)iPDPT & 0x1FF) << 30 \
    | ((u64)iPD & 0x1FF) << 21   \
    | ((u64)iPT & 0x1FF) << 12))

enum Level {
    L_PML4 = 4,
    L_PDPT = 3,
    L_PD   = 2,
    L_PT   = 1,
    L_PG   = 0,
};

/* Locate entry or others by virtual address */
#define PML4E_IDX(Addr) (((UINT64)Addr >> 39) & 0x1FF)
#define PDPTE_IDX(Addr) (((UINT64)Addr >> 30) & 0x1FF)
#define PDE_IDX(Addr)   (((UINT64)Addr >> 21) & 0x1FF)
#define PTE_IDX(Addr)   (((UINT64)Addr >> 12) & 0x1FF)

/* Get vrt addr of pgt by Addr & Level */
static inline u64*
_ReEntryGet (u64 Addr, int Level)
{
    if (Level == L_PML4) {
        return (u64 *)VRT(R_IDX, R_IDX, R_IDX, R_IDX);
    } else if (Level == L_PDPT) {
        return (u64 *)VRT(R_IDX, R_IDX, R_IDX, PML4E_IDX(Addr));
    } else if (Level == L_PD) {
        return (u64 *)VRT(R_IDX, R_IDX, PML4E_IDX(Addr), PDPTE_IDX(Addr));
    } else if (Level == L_PT) {
        return (u64 *)VRT(R_IDX, PML4E_IDX(Addr), PDPTE_IDX(Addr), PDE_IDX(Addr));
    }

    PANIC ("Unable to get the Vrt of pgt by Addr & Level - %p, %d\n", Addr, Level);
}

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

        Tab[Idx] = PE_S_ADDR(New) | PE_RW | PE_P;

        New = _ReEntryGet(Vrt, Level - 1);
        memset (New, 0, PAGE_SIZ);
    }
    Tab = (u64 *)_ReEntryGet(Vrt, Level - 1);

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
    } else if (Mode == MAP_1G) {
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

static void
_UnmapWalk (u64 Vrt, int Level, int Mode)
{
    u64 *Entry = &_ReEntryGet (Vrt, Level)[IDX(Vrt, Level)];
    if (!(*Entry & PE_P)) {
        return;
    }

    if (Level == Mode) {
        *Entry &= ~PE_P;
        __asm__ volatile ("invlpg (%0)" : : "r"(Vrt) : "memory");
        return;
    }

    _UnmapWalk (Vrt, --Level, Mode);
}

void VMMap_Umap (u64 Vrt, size_t Num, int Mode)
{
    DEBUGK ("Try to unmap %p - %llu,%d\n", Vrt, Num, Mode);

    u64 PageSiz = _PageSiz (Mode);
    while (Num--) {
        _UnmapWalk (Vrt, L_PML4, Mode);
        Vrt += PageSiz;
    }
}

#include <Boot.h>

extern void __ConfigSave ();
extern void __Apic_SwitchMode ();
extern void __Graphics_SwitchMode ();

void VMMap_EarlyInit ()
{
    PML4 = (u64 *)ReadCr3();
    PML4[R_IDX] = PE_S_ADDR((u64)PML4) | PE_RW | PE_P;

    // Set pgt into vrt mode
    PML4 = _ReEntryGet(0, L_PML4);

    /* After set recursive pagetable, we can use VMMap to set mapping by recursive way, even if
       we haven't remove the PML4Es for user space, that's ident mapping of physical memory     */

    VMMap (_BootConfig.Graphics.FrameBuffer,
           KERN_FB, DIV_ROUND_UP (_BootConfig.Graphics.FrameBufferSize, PAGE_SIZ),
           PE_RW | PE_P | PTE_G, MAP_4K
           );

    // As callback functions
    __Apic_SwitchMode();
    __Graphics_SwitchMode();
}

void VMMap_Init ()
{
    for (int i = 0; i < 256 ;i++) {
        PML4[i] &= ~PE_P;
    }
}
