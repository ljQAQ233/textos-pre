#include <TextOS/Lib/Ring.h>
#include <TextOS/Memory/Malloc.h>

#include <TextOS/Assert.h>

#include <string.h>

/* If Buffer is not provided, allocate memory automatically */
Ring_t *RingInit (Ring_t *Ring, void *Buffer, size_t Siz, size_t Elem)
{
    ASSERTK (Siz != 0);

    // if (Siz / Elem == 0)
    //     return NULL;

    if (!Buffer && !(Buffer = MallocK (Siz)))
        return NULL;
    
    if (!Ring && !(Ring = MallocK (sizeof(Ring_t))))
        return NULL;
    
    Ring->Buffer = Buffer;
    Ring->Siz    = Siz;
    Ring->Elem   = Elem;
    Ring->Max    = Siz / Elem;

    RingClear (Ring);

    return Ring;
}

/* Fix index (adjust it into correct one) */
static size_t RingFixi (Ring_t *Ring, size_t *Idx)
{
    *Idx %= Ring->Max;
    return *Idx;
}

/* Get the target by index */
void *RingGet (Ring_t *Ring, size_t Idx)
{
    RingFixi (Ring, &Idx);
    return Ring->Buffer + Ring->Elem * Idx;
}

void *RingHead (Ring_t *Ring)
{
    return RingGet (Ring, RingFixi (Ring, &Ring->Head));
}

void *RingTail (Ring_t *Ring)
{
    return RingGet (Ring, RingFixi (Ring, &Ring->Tail));
}

void *RingPop (Ring_t *Ring)
{
    /* Fetch it and update info */
    void *Elem = RingHead (Ring);

    RingFixi (Ring, &Ring->Head);

    Ring->Head++;

    return Elem;
}

void RingPush (Ring_t *Ring, void *Elem)
{
    RingFixi (Ring, &Ring->Tail);

    memcpy (RingGet (Ring, Ring->Tail), Elem, Ring->Elem);
    Ring->Tail++;
}

bool RingEmpty (Ring_t *Ring)
{
    return RingFixi (Ring, &Ring->Head) == RingFixi (Ring, &Ring->Tail);
}

void RingClear (Ring_t *Ring)
{
    memset (Ring->Buffer, 0, Ring->Max);

    Ring->Head = 0;
    Ring->Tail = 0;
}
