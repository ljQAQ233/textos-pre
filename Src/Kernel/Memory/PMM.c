#include <TextOS/Memory.h>
#include <TextOS/Debug.h>
#include <TextOS/Assert.h>
#include <TextOS/Uefi.h>

#include <Boot.h>

#include <string.h>

static u64 PageFree;

/* Mark which zones are free,every node is so small that can be placed in a page easily */
struct FreeNode {
    u64 PageNum;
    struct FreeNode *Next;
};
typedef struct FreeNode FreeNode_t;

static FreeNode_t _Free;

void PMM_Init ()
{
    MAP_INFO *Info = _BootConfig.Memory.Map;
    EFI_MEMORY_DESCRIPTOR *Ptr = Info->Maps + Info->DescSiz; // Skip the first one,its ptr points to NULL.

    FreeNode_t *Node = &_Free;
    for (u64 i = 1;i < Info->MapCount;i++, Ptr = OFFSET(Ptr, Info->DescSiz))
    {
        DEBUGK ("Desc % 3llu : Phy - %016llx , PgSiz - %llu\n", i, Ptr->PhysicalStart, Ptr->NumberOfPages);
        // if (Ptr->Type == EfiBootServicesData ||
        //     Ptr->Type == EfiBootServicesCode)
        //     Ptr->Type  = EfiConventionalMemory;

        if (Ptr->Type != EfiConventionalMemory)
            continue;
        PageFree += Ptr->NumberOfPages;

        /* 处理与内核占用的物理内存的重叠区域 */
        int64 DeltaStart = Ptr->PhysicalStart - KERN_PHY;
        int64 DeltaEnd   = Ptr->PhysicalStart + Ptr->NumberOfPages * PAGE_SIZ - (KERN_PHY + KERN_ATC);
        if (DeltaStart > 0 && ABS(DeltaStart) < KERN_ATC && Ptr->NumberOfPages) {               // on the right
            if (DeltaEnd > 0) {
                goto RangeBrk;
            } else {
                continue;
            }
        }
        else if (DeltaStart < 0 && ABS(DeltaStart) + KERN_ATC < Ptr->NumberOfPages * PAGE_SIZ) { // on the left
            Node->Next = (FreeNode_t *)Ptr->PhysicalStart;
            Node = Node->Next;
            Node->PageNum = (KERN_PHY - Ptr->PhysicalStart) >> 12;

            if (DeltaEnd > 0) {
                goto RangeBrk;
            }
            continue;
        }
        else if (DeltaStart == 0 && Ptr->NumberOfPages) {                                            // at the same place
            if (DeltaEnd > 0) {
RangeBrk:
                Node->Next = (FreeNode_t *)(KERN_PHY + KERN_ATC);
                Node = Node->Next;
                Node->PageNum = (u64)DeltaEnd >> 12;

                continue;
            }
        }

        Node->Next = (FreeNode_t *)Ptr->PhysicalStart;
        Node = Node->Next;
        Node->PageNum = Ptr->NumberOfPages;
    }
}

/* 获取 Num 页的内存,没有则返回 NULL */
void *PMM_AllocPages (size_t Num)
{
    void *Page = NULL;

    FreeNode_t *Node = &_Free;
    FreeNode_t *Prev = &_Free;

    do {
        Node = Node->Next;
        if (Node->PageNum >= Num)
        {
            Node->PageNum -= Num;
            Page = (void *)Node;

            if (Node->PageNum == 0) {
                Prev->Next = Node->Next; // 越过,销毁
            } else {
                FreeNode_t *New = OFFSET(Node, Num * PAGE_SIZ); // 偏移 Node

                New->Next = Node->Next;
                New->PageNum = Node->PageNum - Num;

                Prev->Next = New;                              // 校正 Prev
            }

            break;
        }
        Prev = Prev->Next;
    } while (Node->Next);
    DEBUGK ("Allocate pages ! - %p,%llu\n", Page,Num);

    return Page;
}

static bool _PMM_IsFree (void *Page, size_t Num)
{
    FreeNode_t *Node = &_Free;

    /*
       ZoneA : ---------xxxxxxxxx
       ZoneB : xxxxxxxxx---------

       non   : ZAStart >= ZBEnd || ZBStart >= ZAEnd
       exist(for `if`) : MAX(ZAStart, ZBStart) < MIN(ZAEnd, ZBEnd)
    */

    u64 Start, End;

    do {
        Node = Node->Next;
        Start = (u64)Node;
        End   = (u64)Node + Node->PageNum * PAGE_SIZ;

        if (MAX(Start, (u64)Page) < MIN(End, (u64)Page + Num * PAGE_SIZ))
        {
            return true;
        }

    } while (Node->Next);
    return false;
}

void PMM_FreePages (void *Page,size_t Num)
{
    Page = (void*)((u64)Page &~ 0x7ff); // 抹掉低位
    if (_PMM_IsFree (Page, Num)) {
        DEBUGK ("Theses page are free before! - %p,%llu\n", Page, Num);
        return;
    }

    FreeNode_t *Node = &_Free;
    FreeNode_t *Prev = &_Free;
    do {
        Node = Node->Next;

        if ((u64)Node == (u64)Page + Num * PAGE_SIZ) // Head
        {
            FreeNode_t *New = Page;

            New->PageNum = Node->PageNum + Num;
            New->Next = Node->Next;
            Prev->Next = New;
            break;
        }
        else if ((u64)Node + Node->PageNum * PAGE_SIZ == (u64)Page) // Tail
        {
            Node->PageNum += Num;
            break;
        }
        else if ((u64)Node < (u64)Page && (u64)Node->Next > (u64)Page) // Another case
        {
            FreeNode_t *New = Page;

            New->PageNum = Num;
            New->Next = Node->Next;

            Node->Next = New;
            break;
        }

        Prev = Prev->Next;
    } while (Node->Next);
    DEBUGK ("Free pages! - %p,%llu\n", Page, Num);
}

