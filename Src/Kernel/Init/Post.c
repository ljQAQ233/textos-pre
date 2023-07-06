#include <Boot.h>
#include <TextOS/Graphics.h>
#include <TextOS/Memory/Map.h>
#include <TextOS/Memory/VMM.h>

#include <string.h>

#define _ADJUST(Tgt) (Tgt = (__typeof__(Tgt))((void *)Tgt - _BootConfig.Args.Data + KERN_BCFG))

void __ConfigSave ()
{
    VMM_PhyAuto (KERN_BCFG, _BootConfig.Args.PgNum, PE_P | PE_RW);

    memcpy ((void *)KERN_BCFG, _BootConfig.Args.Data, PAGE_SIZ * _BootConfig.Args.PgNum);

    _ADJUST(Font);
    _ADJUST(Font->Base);
}
