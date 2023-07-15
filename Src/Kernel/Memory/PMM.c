/*
   NOTE : We must switch mode to allocate memory for noides before
          break the lower memory space down!
*/

#include <TextOS/Memory.h>
#include <TextOS/Memory/Malloc.h>
#include <TextOS/Debug.h>
#include <TextOS/Assert.h>
#include <TextOS/Uefi.h>

#include <Boot.h>

#include <string.h>

static u64 PageFree;

/* Mark which zones are free,every node is so small that can be placed in a page easily */
struct FreeNode {
    u64 PageNum;
    u64 Addr;
    struct FreeNode *Next;
};
typedef struct FreeNode FreeNode_t;

static FreeNode_t _Free;

typedef void * (*NewNode_t) (void *Node, size_t Num);
typedef void   (*DelNode_t) (void *Node);
/*
   Ways to allocate memory for nodes:
   1. Put FreeNode in free pages directly
   2. MallocK!!!
*/
static NewNode_t NewNode;
static DelNode_t DelNode;
static void *EarlyNewNode (void *Node, size_t Num) { return Node + Num * PAGE_SIZ; }
static void *AfterNewNode (void *Node, size_t Num) { return MallocK (sizeof(FreeNode_t)); }
static void EarlyDelNode (void *Node) {};
static void AfterDelNode (void *Node) { FreeK (Node); }
static void _PMM_SwitchMode () { NewNode = AfterNewNode; DelNode = AfterDelNode; }

void PMM_EarlyInit ()
{
    MAP_INFO *Info = _BootConfig.Memory.Map;
    EFI_MEMORY_DESCRIPTOR *Ptr = Info->Maps + Info->DescSiz; // Skip the first one,its ptr points to NULL.

    FreeNode_t *Node = &_Free;
    for (u64 i = 1;i < Info->MapCount;i++, Ptr = OFFSET(Ptr, Info->DescSiz))
    {
        DEBUGK ("Desc % 3llu : Phy - %016llx , PgSiz - %llu\n", i, Ptr->PhysicalStart, Ptr->NumberOfPages);

        /* 回收 启动时服务的内存 的工作交给后辈... */

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
            Node->Addr = (u64)Node;
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
                Node->Addr = (u64)Node;
                Node->PageNum = (u64)DeltaEnd >> 12;

                continue;
            }
        }

        Node->Next = (FreeNode_t *)Ptr->PhysicalStart;
        Node = Node->Next;
        Node->Addr = (u64)Node;
        Node->PageNum = Ptr->NumberOfPages;
    }

    NewNode = (NewNode_t)EarlyNewNode;
    DelNode = (DelNode_t)EarlyDelNode;
}

/*
   Will be called after the early initialization(`PMM_EarlyInit`);
   Use new method to describe physical memory, this case uses `MallocK`.
   So the first thing is making sure the old structure(_Free.Next) will
   not be changed instead of filling it.
*/
void PMM_Init ()
{
    FreeNode_t *Old = &_Free,
               *New = &_Free;

    int c = 0, i = 0;
    for (FreeNode_t *p = _Free.Next; p ;p = p->Next, c++) ;
    void *Array[c];
    for (i = 0; i < c ;i++)
        Array[i] = MallocK (sizeof(FreeNode_t));

    i = 0;
    while (Old->Next && i < c) {
        Old = Old->Next;

        New->Next = Array[i++];
        New = New->Next;

        New->Addr = Old->Addr;
        New->PageNum = Old->PageNum;
    }

    while (i < c)
        FreeK (Array[i++]);

    _PMM_SwitchMode ();

    /* 接下来是回收 启动时服务 的内存空间 */
    MAP_INFO *Info = _BootConfig.Memory.Map;
    EFI_MEMORY_DESCRIPTOR *Ptr = Info->Maps + Info->DescSiz; // ...之前已经解释过了...
    for (u64 i = 1;i < Info->MapCount;i++, Ptr = OFFSET(Ptr, Info->DescSiz))
    {
        if (Ptr->Type == EfiBootServicesData ||
            Ptr->Type == EfiBootServicesCode)
            PMM_FreePages ((void *)Ptr->PhysicalStart, Ptr->NumberOfPages);
    }
}

static void _PMM_AllocHard (void *Page)
{
    FreeNode_t *Node = &_Free;
    FreeNode_t *Prev = &_Free;
    do {
        Node = Node->Next;

        if ((u64)Page == Node->Addr) {
            if (Node->PageNum == 1) {
                Prev->Next = Node->Next;
                break;
            }
            FreeNode_t *New = NewNode (Node, 1);

            New->Addr = Node->Addr + PAGE_SIZ;
            New->Next = Node->Next;
            New->PageNum = Node->PageNum - 1;

            Prev->Next = New;
            DelNode (Node);
            break;
        } else if ((u64)Page == Node->Addr + Node->PageNum * PAGE_SIZ) {
            Node->PageNum--;
            if (Node->PageNum == 0) {
                Prev->Next = Node->Next;
                DelNode (Node);
            }
            break;
        } else if (Node->Addr < (u64)Page 
                    && (u64)Page < Node->Addr + Node->PageNum * PAGE_SIZ) {
            u64 PgOffset = ((u64)Page - Node->Addr) / PAGE_SIZ;
            FreeNode_t *New = NewNode (Node, PgOffset + 1);

            New->Addr = (u64)Page + PAGE_SIZ;
            New->PageNum = Node->PageNum - PgOffset - 1;
            New->Next = Node->Next;

            Node->Next = New;
            Node->PageNum = PgOffset;

            break;
        }

        Prev = Prev->Next;
    } while (Node->Next);
}

void PMM_AllocHard (void *Page, size_t Num)
{
    Page = (void*)((u64)Page &~ 0x7ff); // 抹掉低位

    DEBUGK ("Try to allocate pages(force) ! - %p\n", Page);
    while (Num--) {
        _PMM_AllocHard (Page);
        Page += PAGE_SIZ;
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
            Page = (void *)Node->Addr;

            if (Node->PageNum == 0) {
                Prev->Next = Node->Next; // 越过,销毁
                DelNode (Node);
            } else {
                FreeNode_t *New = NewNode (Node, Num);         // 设置 Node

                New->Next = Node->Next;
                New->Addr = Node->Addr + Num * PAGE_SIZ;
                New->PageNum = Node->PageNum - Num;

                Prev->Next = New;                              // 校正 Prev
                DelNode (Node);
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
        Start = (u64)Node->Addr;
        End   = (u64)Node->Addr + Node->PageNum * PAGE_SIZ;

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

        if ((u64)Node->Addr == (u64)Page + Num * PAGE_SIZ) // Head
        {
            FreeNode_t *New = NewNode (Page, 0);

            New->PageNum = Node->PageNum + Num;
            New->Addr = (u64)Page;
            New->Next = Node->Next;
            Prev->Next = New;
            break;
        }
        else if ((u64)Node->Addr + Node->PageNum * PAGE_SIZ == (u64)Page) // Tail
        {
            Node->PageNum += Num;
            break;
        }
        else if ((u64)Node->Addr < (u64)Page && (u64)Node->Next > (u64)Page) // Another case
        {
            FreeNode_t *New = NewNode (Page, 0);

            New->PageNum = Num;
            New->Addr = (u64)Page;
            New->Next = Node->Next;

            Node->Next = New;
            break;
        }

        Prev = Prev->Next;
    } while (Node->Next);
    DEBUGK ("Free pages! - %p,%llu\n", Page, Num);
}

