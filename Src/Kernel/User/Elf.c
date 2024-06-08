#include <TextOS/FileSys.h>
#include <TextOS/User/Elf.h>
#include <TextOS/Memory.h>
#include <TextOS/Memory/VMM.h>
#include <TextOS/Memory/Map.h>
#include <TextOS/Memory/Malloc.h>

#define ERR_RETS(Stat) \
    if (Stat < 0)      \
        return Stat;   \

#define A(Expr)       \
    if (!(Expr))      \
        return false;

bool ElfCheck (Hdr64_t *Hdr)
{
    A(Hdr->Magic == ELF_MAGIC);
    A(Hdr->Type == ET_EXEC);
    A(Hdr->Class == ELF_SUPPORTED_CLASS);
    A(Hdr->Machine == ELF_SUPPORTED_ARCH);

    return true;
}

#undef A

#include <TextOS/ErrNo.h>
#include <string.h>

#define ALIGN_UP(Target, Base) ((Base) * ((Target + Base - 1) / Base))

int ElfLoad (char *Path, Exec_t *Info)
{
    int Stat = 0;
    Node_t *Node = NULL;

    Stat = __VrtFs_Open (NULL, &Node, Path, O_READ);
    ERR_RETS(Stat);

    Hdr64_t *Hdr = MallocK (sizeof(Hdr64_t));
    Stat = __VrtFs_Read (Node, Hdr, sizeof(Hdr64_t), 0);
    ERR_RETS(Stat);

    if (!ElfCheck (Hdr))
        return -ENOEXEC;
    
    ProgHdr64_t *Progs = MallocK (sizeof(ProgHdr64_t) * Hdr->PhNum);
    Stat = __VrtFs_Read (Node, Progs, sizeof(ProgHdr64_t) * Hdr->PhNum, Hdr->PhOffset);
    ERR_RETS(Stat);
    
    int Load = 0;
    for (int i = 0 ; i < Hdr->PhNum ; i++) {
        ProgHdr64_t *Prog = &Progs[i];
        if (Prog->Type != PT_LOAD)
            continue;

        VMM_PhyAuto (
            Prog->VirtualAddr,
            DIV_ROUND_UP(Prog->MemSiz, PAGE_SIZ),
            PE_P | PE_US | PE_RW
            );
        void *Buffer = (void *)Prog->VirtualAddr;
        memset (Buffer, 0, DIV_ROUND_UP(Prog->MemSiz, PAGE_SIZ));
        Stat = __VrtFs_Read (Node, Buffer, Prog->FileSiz, Prog->Offset);
        ERR_RETS(Stat);

        Load++;
    }

    /* 一个可以加载的段都没有... */
    if (Load == 0)
        return -ENOEXEC;

    Info->Entry = Hdr->Entry;

    return 0;
}
