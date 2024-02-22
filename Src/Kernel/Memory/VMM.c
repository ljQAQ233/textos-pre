#include <TextOS/Memory.h>
#include <TextOS/Memory/Map.h>

#include <TextOS/Debug.h>
#include <TextOS/Assert.h>

/* Check if the Vrt is a canonical format
   vrt addr and adjust it if it is invalid

   @retval  int   The state */
int VMM_CaAdjust (u64 *Vrt)
{
    if ((*Vrt >> 47) & 1) {
        if (*Vrt >> 48 == 0xFFFF) {
            return true;
        }

        *Vrt |= 0xFFFFULL << 48;
        return false;
    }

    return true;
}

void *VMM_PhyAuto (u64 Vrt, size_t Num, u16 Flgs)
{
    ASSERTK (!(Vrt & 0xFFFF) && Vrt != 0); // 确保它不是 NULL 并且是一页开始的地方

    if (!VMM_CaAdjust (&Vrt)) {
        DEBUGK ("The addr is not a canonical addr, adjust it - %p\n",Vrt);
    }

    void *Page = PMM_AllocPages (Num);

    VMMap ((u64)Page, Vrt, Num, Flgs, MAP_4K);

    return (void *)Vrt;
}

static u64 _Idx;

/* TODO: complete it */
void *VMM_AllocPages (size_t Num, u16 Flgs)
{
    void *Page = (void *)KERN_VMM + PAGE_SIZ * _Idx;

    _Idx += Num;

    VMM_PhyAuto ((u64)Page, Num, Flgs);

    return Page;
}

/* TODO: Design memory space and set up VMM */
