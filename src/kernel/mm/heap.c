#include <textos/mm.h>
#include <textos/mm/map.h>

#define HEAP_ORIG 1

typedef u32 block_t;

// This way is easier to manage
typedef struct {
    void   *brk;   // 指向结尾块,Break 指针
    void   *root;  // 指向第一个块头,即序言块后的第一个头
    void   *max;   // 虚拟内存中堆的有效最大地址
} Heap_t;

#define F_MASK 0x7 // Flags
#define A_MASK 0x1 // A/F -> Allocated / Free

#define SIZ(blk) ((block_t)(blk) >> 3)      // to get the size of this block including hdr & ftr
#define ALOC(blk) ((block_t)(blk) & A_MASK) // to get A/F

/*
   This `siz` describes how many bytes the block takes up,
   including hdr & ftr.when get it,it must be right shifted like we saied before!
   Other bits are reserved now.
*/
#define SET(siz, aloc) (((siz) << 3) | (aloc & A_MASK))

/* Put hdr/ftr to addr */
#define PUT(addr, Var) ((*(block_t *)(addr)) = (block_t)Var)

#define PAYLOAD(hdr) ((void *)(hdr) + sizeof(block_t))

/* Round up by alignment, e.g. _ALIGN(1, 8) => 1 -> 8 */
#define _ALIGN(num, alignment) (((num) + (alignment) - 1) & ~((alignment) - 1))

static Heap_t _heap = {NULL, NULL, NULL};

static void _merge(block_t *hdr);
static void _extend_heap(size_t siz);

void heap_init()
{
    block_t *hp = pmm_allocpages(HEAP_ORIG); // 分配初始页,总不可能让指针们踩空吧......

    PUT (hp + 0, SET(0, 0));    // Padding
    PUT (hp + 1, SET(8, true)); // 设置序言块
    PUT (hp + 2, SET(8, true));
    PUT (hp + 3, SET(0, true)); // 设置结尾块 has only header

    vmap_map ((u64)hp, __kern_heap_base, HEAP_ORIG, PE_RW | PE_P, MAP_4K); // 映射至内核堆处

    _heap.root = (block_t *)__kern_heap_base + 3;
    _heap.brk = (block_t *)_heap.root;
    _heap.max = (void *)__kern_heap_base + HEAP_ORIG * PAGE_SIZ;
}

void *sbrk(int64 siz)
{
    _heap.brk += _ALIGN(siz, 8);

    return (_heap.brk - siz);
}

#include <textos/assert.h>

void *malloc(size_t siz)
{
    if (siz == 0)
        return NULL;
    siz = _ALIGN(siz + sizeof(block_t) * 2, 8);
    
    block_t *curr = _heap.root, *next;
    for (;;)
    {
        if (!ALOC(*curr)) {
            if (SIZ(*curr) > siz) {
                PUT(OFFSET(curr, SIZ(*curr) - 4), SET(SIZ(*curr) - siz, false)); // Next free footer
                PUT(OFFSET(curr, siz           ), SET(SIZ(*curr) - siz, false)); // Next free header

                PUT(curr, SET(siz, true));                  // Header
                PUT(OFFSET(curr, siz - 4), SET(siz, true)); // Footer

                return PAYLOAD (curr);
            } else if (SIZ(*curr) == siz) {
                PUT(curr, SET(siz, true));                 // Header
                PUT(OFFSET(curr, SIZ(*curr) - 4),*curr);   // Footer

                return PAYLOAD (curr);
            }
        }

        next = OFFSET(curr, SIZ(*curr));
        if (!((u64)next < (u64)_heap.brk))
            _extend_heap(siz);
        else
            curr = next;
    }

    return NULL;
}

#include <string.h>

void *realloc(void *addr, size_t newsiz)
{
    if (!addr)
        return malloc(newsiz);

    if (!newsiz)
    {
        free(addr);
        return NULL;
    }

    block_t *hdr = addr - 4;
    size_t siz = SIZ(*hdr);

    // do nothing
    // TODO : replace it
    if (siz >= newsiz)
        return addr;
    else
    {
        void *newmem = malloc(newsiz);
        memcpy(newmem, addr, siz);
        free(addr);
        return newmem;
    }

    // unreachable
    return NULL;
}

void *calloc(size_t siz)
{
    void *ptr = malloc(siz);
    if (!ptr)
        return ptr;
    return memset(ptr, 0, siz);
}

void free (void *addr)
{
    if (addr == NULL || (u64)addr > (u64)_heap.max)
        return;

    block_t *hdr = addr - 4;
    block_t *ftr = (void *)hdr + SIZ(*hdr) - sizeof(block_t);

    if (*hdr != *ftr)
        return;

    ASSERTK(ALOC(*hdr));

    PUT(hdr, SET(SIZ(*hdr), false));
    PUT(ftr, SET(SIZ(*ftr), false));

    // ૮(˶ᵔ ᵕ ᵔ˶)ა
    _merge(hdr);
}

static void _merge(block_t *hdr)
{
    block_t *ftr = (void *)hdr + SIZ(*hdr) - sizeof(block_t);
    
    block_t *prev_ftr = hdr - 1;
    block_t *next_hdr = ftr + 1;

    if (!ALOC(*next_hdr) && SIZ(*next_hdr)) {
        block_t *next_ftr = OFFSET (next_hdr, SIZ(*next_hdr) - 4);
        ASSERTK(*next_hdr == *next_ftr);
        PUT(hdr, SET(SIZ(*hdr) + SIZ(*next_hdr), false)); // Set hdr
        PUT(next_ftr, *hdr);                              // Set Ftr
    }

    if (!ALOC(*prev_ftr) && SIZ(*prev_ftr)) {
        block_t *prev_hdr = (void *)hdr - SIZ(*prev_ftr);
        ASSERTK(*prev_hdr == *prev_ftr);
        PUT(prev_hdr, SET(SIZ(*prev_hdr) + SIZ(*hdr), false));
        // 到这里, ftr 可能已经改变了. 所以不能直接写 ftr
        PUT(OFFSET(prev_hdr, SIZ(*prev_hdr) - 4), *prev_hdr);
    }
}

static void _extend_heap(size_t siz)
{
    siz = _ALIGN(siz, 8);
    int64 diff = (u64)_heap.brk + siz - (u64)_heap.max;
    if (diff > 0) {
        u64 pages = DIV_ROUND_UP(diff, PAGE_SIZ);
        void *ext = pmm_allocpages(pages);

        vmap_map((u64)ext, (u64)_heap.max, pages, PE_RW | PE_P, MAP_4K);

        _heap.max += pages * PAGE_SIZ;
    }

    void *hdr = sbrk(siz); // get prev Brk as hdr that a new free block holds

    PUT(hdr, SET(siz, false));           // 新的头
    PUT(hdr + siz - 4, SET(siz, false)); // 新的脚
    PUT(hdr + siz, SET(0, true));        // 结尾块

    _merge(hdr);
}
