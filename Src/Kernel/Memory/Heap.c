#include <TextOS/Memory.h>
#include <TextOS/Memory/Map.h>

#define HEAP_ORIG 1

typedef u32 Block_t;

// This way is easier to manage
typedef struct {
    void   *Brk;   // 指向结尾块,Break 指针
    void   *Root;  // 指向第一个块头,即序言块后的第一个头
    void   *Max;   // 虚拟内存中堆的有效最大地址
} Heap_t;

#define F_MASK 0x7 // Flags
#define A_MASK 0x1 // A/F -> Allocated / Free

#define SIZ(Blk) ((Block_t)(Blk) >> 3)      // to get the size of this block including hdr & ftr
#define ALOC(Blk) ((Block_t)(Blk) & A_MASK) // to get A/F

/*
   This `Siz` describes how many bytes the block takes up,
   including hdr & ftr.when get it,it must be right shifted like we saied before!
   Other bits are reserved now.
*/
#define SET(Siz, Aloc) (((Siz) << 3) | (Aloc & A_MASK))

/* Put hdr/ftr to Addr */
#define PUT(Addr, Var) ((*(Block_t *)(Addr)) = (Block_t)Var)

#define PAYLOAD(Hdr) ((void *)(Hdr) + sizeof(Block_t))

/* Round up by Alignment, e.g. _ALIGN(1, 8) => 1 -> 8 */
#define _ALIGN(Num, Alignment) (((Num) + (Alignment) - 1) & ~((Alignment) - 1))

static Heap_t _Heap = {NULL, NULL, NULL};

static void _Merge (Block_t *Hdr);
static void _ExtendHeap (size_t Siz);

void Heap_Init ()
{
    Block_t *Heap = PMM_AllocPages (HEAP_ORIG); // 分配初始页,总不可能让指针们踩空吧......

    PUT (Heap + 0, SET(0, 0));    // Padding
    PUT (Heap + 1, SET(8, true)); // 设置序言块
    PUT (Heap + 2, SET(8, true));
    PUT (Heap + 3, SET(0, true)); // 设置结尾块 has only header

    VMMap ((u64)Heap, KERN_HEAPV, HEAP_ORIG, PE_RW | PE_P, MAP_4K); // 映射至内核堆处

    _Heap.Root = (Block_t *)KERN_HEAPV + 3;
    _Heap.Brk = (Block_t *)_Heap.Root;
    _Heap.Max = (void *)KERN_HEAPV + HEAP_ORIG * PAGE_SIZ;
}

void *SBrk (int64 Siz)
{
    _Heap.Brk += _ALIGN (Siz, 8);

    return (_Heap.Brk - Siz);
}

#include <TextOS/Assert.h>

void *MallocK (size_t Siz)
{
    u64 Req = Siz;

    if (Siz == 0) {
        return NULL;
    }
    Block_t *Curr = _Heap.Root;

    Siz = _ALIGN(Siz + sizeof(Block_t) * 2, 8);
    while (Curr && (u64)Curr < (u64)_Heap.Brk) {
        if (!ALOC(*Curr)) {
            if (SIZ(*Curr) > Siz) {
                PUT (OFFSET(Curr, SIZ(*Curr) - 4), SET(SIZ(*Curr) - Siz, false)); // Next free footer
                PUT (OFFSET(Curr, Siz           ), SET(SIZ(*Curr) - Siz, false)); // Next free header

                PUT (Curr, SET(Siz, true));                  // Header
                PUT (OFFSET(Curr, Siz - 4), SET(Siz, true)); // Footer

                return PAYLOAD (Curr);
            } else if (SIZ(*Curr) == Siz) {
                PUT (Curr, SET(Siz, true));                 // Header
                PUT (OFFSET(Curr, SIZ(*Curr) - 4),*Curr);   // Footer

                return PAYLOAD (Curr);
            }
        }

        ASSERTK (*Curr != 0);
        Curr = OFFSET (Curr, SIZ(*Curr));
    }

    _ExtendHeap (Siz);
    
    return MallocK (Req);
}

void FreeK (void *Addr)
{
    if (Addr == NULL || (u64)Addr > (u64)_Heap.Max) {
        return;
    }

    Block_t *Hdr = Addr - 4;
    Block_t *Ftr = (void *)Hdr + SIZ(*Hdr) - sizeof(Block_t);

    if (*Hdr != *Ftr) {
        return;
    }

    PUT(Hdr, SET(SIZ(*Hdr), false));
    PUT(Ftr, SET(SIZ(*Ftr), false));

    // ૮(˶ᵔ ᵕ ᵔ˶)ა
    _Merge (Hdr);
}

static void _Merge (Block_t *Hdr)
{
    Block_t *Ftr = (void *)Hdr + SIZ(*Hdr) - sizeof(Block_t);
    
    Block_t *PrevFtr = Hdr - 1;
    Block_t *NextHdr = Ftr + 1;

    if (!ALOC(*NextHdr) && SIZ(*NextHdr)) {
        Block_t *NextFtr = OFFSET (NextHdr, SIZ(*NextHdr) - 4);
        ASSERTK (*NextHdr == *NextFtr);
        PUT (Hdr, SET(SIZ(*Hdr) + SIZ(*NextHdr), false)); // Set Hdr
        PUT (NextFtr, *Hdr);                              // Set Ftr
    }
    if (!ALOC(*PrevFtr) && SIZ(*PrevFtr)) {
        Block_t *PrevHdr = (void *)Hdr - SIZ(*PrevFtr);
        ASSERTK (*PrevHdr == *PrevFtr);
        PUT (PrevHdr, SET(SIZ(*PrevHdr) + SIZ(*Hdr), false));
        PUT (Ftr, *PrevHdr);
    }
}

static void _ExtendHeap (size_t Siz)
{
    Siz = _ALIGN (Siz, 8);
    int64 Diff = (u64)_Heap.Brk + Siz - (u64)_Heap.Max;
    if (Diff > 0) {
        u64 Pages = DIV_ROUND_UP(Diff, PAGE_SIZ);
        void *Ext = PMM_AllocPages(Pages);

        VMMap ((u64)Ext, (u64)_Heap.Max, Pages, PE_RW | PE_P, MAP_4K);

        _Heap.Max += Pages * PAGE_SIZ;
    }

    void *Hdr = SBrk (Siz); // get prev Brk as Hdr that a new free block holds

    PUT (Hdr, SET(Siz, false));           // 新的头
    PUT (Hdr + Siz - 4, SET(Siz, false)); // 新的脚
    PUT (Hdr + Siz, SET(0, true));        // 结尾块

    _Merge (Hdr);
}
