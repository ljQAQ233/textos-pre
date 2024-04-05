#include "Base.h"
#include "Types.h"

#include <string.h>

/* Biso parameter block */
typedef struct _packed
{
    u8          JmpInstr[3];        // Jump instruction
    char        OemId[8];           // OEM identifier
    u16         SecSiz;             // The num of bytes per sector
    u8          SecPerCluster;      // The num of clusters per sector
    u16         SecReserved;        // The num of reserved sectors
    u8          FatNum;             // The num of File Allocation Tables, often is 2
    u16         Roots;              // The num of entries in root, 0 for fat32!
    u16         SecNum16;           
    u8          MediaDesc;          
    u16         FatSiz16;           
    u16         SecPerTrack;        
    u16         Heads;              // The num of heads
    u32         SecHide;            // The num of the hidden sectors
    u32         SecNum32;           
    u32         FatSiz32;           
    u16         ExtFlgs;            // Extended flags
    u16         FatVer;             
    u32         RootClus;           // The start cluster of root
    u16         Info;               // The information of this file system
    u16         BootBackup;         // The backup sector of MBR
    u8          Reserved1[12];      
    u8          DriveNum;           
    u8          Reserved2;          
    u8          ExtBootSig;         
    u32         VolId;              
    char        VolLabel[11];       
    char        FatType[8];         
    u8          Binary[356];        // Boot code
    Partition_t Partition[4];       // Partition table
    u16         EndSym;             // 0xAA55
} Fat32Record_t;

STATIC_ASSERT(sizeof(Fat32Record_t) == 512, "wrong size");

typedef struct _packed
{
    u8 Day   : 5;
    u8 Month : 4;
    u8 Year  : 7; // 1980 - 2107
} Date_t;

typedef struct _packed
{
    u8 Second : 5; // 0 - 29 -> 0s - 58s
    u8 Minute : 6;
    u8 Hour   : 5;
} Time_t;

STATIC_ASSERT(sizeof(Date_t) == 2, "wrong size");
STATIC_ASSERT(sizeof(Time_t) == 2, "wrong size");

typedef struct _packed
{
    char   Name[11]; // Base(8) + Ext(3)
    u8     Attr;
    u8     Reserved;
    u8     CreateMs;
    Time_t CreateTm;
    Date_t CreateDate;
    Date_t AccessDate;
    u16    ClusterHigh;
    Time_t WriteTm;
    Date_t WriteDate;
    u16    ClusterLow;
    u32    FileSiz;
} Entry_t;

typedef struct _packed
{
    u8  Order;
    u16 Name1[5];
    u8  Attr;
    u8  Type;
    u8  ShortCkSum;
    u16 Name2[6];
    u16 Cluster; // Always zero
    u16 Name3[2];
} EntryLong_t;

STATIC_ASSERT(sizeof(Entry_t) == 32    , "wrong size");
STATIC_ASSERT(sizeof(EntryLong_t) == 32, "wrong size");

#define FA_RO      0x01
#define FA_HIDDEN  0x02
#define FA_SYS     0x04
#define FA_VOLID   0x08
#define FA_DIR     0x10
#define FA_ARCHIVE 0x20
#define FA_LONG    (FA_RO | FA_HIDDEN | FA_SYS | FA_VOLID)

typedef struct
{
    Dev_t *Dev;

    u64           FirstDataSec;
    Record_t      *Mbs;       // The master boot sector of this partition
    Fat32Record_t *Record;    // The boot record of this Fat32 file system
    Partition_t   *Partition; // The partition entry in Mbs
} Info_t;

#include <TextOS/Memory/Malloc.h>

#include <TextOS/Debug.h>
#include <TextOS/Console/PrintK.h>

static Node_t *Fat32_PathWalk (Node_t *Start, char *Path);

static Node_t *Fat32_Open  (Node_t *This, char *Path, u64 Args);
static int     Fat32_Close (Node_t *This);
static int     Fat32_Read  (Node_t *This, void *Buffer, size_t Siz);

FS_INITIALIZER(__FsInit_Fat32)
{
    Fat32Record_t *Record = MallocK(sizeof(Fat32Record_t));
    Hd->BlkRead (Partition->Relative, Record, 1);

    if (Record->EndSym != 0xAA55)
        return NULL;

    u64 FatSiz = Record->FatSiz16 == 0 ? Record->FatSiz32 : Record->FatSiz16;

    u64 FirstDataSec = Record->SecReserved
                     + Record->FatNum * FatSiz
                     + (Record->Roots * 32 + (Record->SecSiz - 1)) / Record->SecSiz; // root_dir_sectors

    u64 Root = (Record->RootClus - 2) * Record->SecPerCluster + FirstDataSec;

    Info_t *Sys = MallocK(sizeof(Info_t));
    Node_t *Ori = MallocK(sizeof(Node_t));

    Sys->Dev = Hd;
    Sys->Mbs = Mbs;
    Sys->Record = Record;
    Sys->Partition = Partition;
    Sys->FirstDataSec = FirstDataSec;

    Ori->Attr = NA_DIR | NA_PROTECT;
    Ori->Name = "/";
    Ori->Root = Ori;
    Ori->Child = NULL;
    Ori->Parent = Ori;
    Ori->Private.Sys = Sys;
    Ori->Private.Addr = Root;

    Ori->Siz = 0;    

    Ori->Opts.Open  = Fat32_Open;
    Ori->Opts.Close = Fat32_Close;
    Ori->Opts.Read  = Fat32_Read;

    // PrintK("Try to pathwalk...\n");
    // if (Fat32_PathWalk(Origin, "/EFI/Boot"))
    //     PrintK("/EFI/Boot was found!!!\n");

    return Ori;
}

/* 拿到的应该是干净的 路径 , 不以 '/' 开头 */
static Node_t *Fat32_Open (Node_t *This, char *Path, u64 Args)
{
    Node_t *Ori = This;

    return Fat32_PathWalk (Ori, Path);
}

/* Close a node , and delete its sub-dir */
static int Fat32_Close (Node_t *This)
{
    if (This->Attr & NA_DIR)
        while (This->Child)
            Fat32_Close (This->Child);

    if (This->Parent != This)
        goto Complete;
    /* 除去父目录项的子目录项 */
    {
        Node_t *Curr = This->Parent->Child;
        Node_t *Prev = This->Parent->Child;

        while (Curr != NULL) {
            Curr = Curr->Next;
            if (Curr == This) {
                Prev->Next = Curr->Next;
                break;
            }
            Prev = Prev->Next;
        } 
    }

Complete:
    FreeK (This->Private.Sys);
    FreeK (This);
}

/* 已经处于一个尴尬的境地了 -> 因为之前已经将 目录 读取进 `_Root` */
static int _ReadDir(Node_t *This, void *Buffer, size_t Count)
{
    int Real = 0;

    for (Node_t *p = This->Child; p != NULL; p = p->Next)
        Real++;

    return Real;
}

static int _ReadContent (Node_t *This, void *Buffer, size_t Siz)
{
    Info_t *Sys = This->Private.Sys;

    int Real = MIN(This->Siz, Siz);
    size_t Count = DIV_ROUND_UP(Real, Sys->Record->SecSiz);

    /* TODO: Replace it */
    void *Tmp = MallocK(Sys->Record->SecSiz * Count);

    Sys->Dev->BlkRead (This->Private.Addr, Tmp, Count);
    memcpy (Buffer, Tmp, Real);

    FreeK(Tmp);

    return Real;
}

static int Fat32_Read (Node_t *This, void *Buffer, size_t Siz)
{
    if (This->Attr & NA_ARCHIVE)
        return _ReadContent (This, Buffer, Siz);

    return _ReadDir (This, NULL, Siz);   // TODO
}

#define _LEN_NAME 8
#define _LEN_EXT 3

#include <string.h>

static inline size_t _ValidChr (const char *Str)
{
    size_t i = 0;

    while (Str[i])
    {
        if (Str[i] == ' ' || Str[i] == '/')
            break;
        i++;
    }

    return i;
}

_UTIL_NEXT();

static Entry_t *_EntryDup (Entry_t *Ori)
{
    Entry_t *Buf = MallocK(sizeof(Entry_t));

    return memcpy (Buf, Ori, sizeof(Entry_t));
}

static u64 _AddrGet (Info_t *Sys, Entry_t *Entry)
{
    u64 Addr = Sys->FirstDataSec
             + Sys->Record->SecPerCluster * ((Entry->ClusterLow | (u32)Entry->ClusterHigh << 16) - 2);

    return Addr;
}

static bool _CmpBuf(char *A, char *B)
{
    size_t LenA = _ValidChr(A),
           LenB = _ValidChr(B);

    if (LenA != LenB)
        return false;

    for (int i = 0; i < LenB; i++)
        if (A[i] != B[i])
            return false;

    return true;
}

/* 长目录项的 文件名 以及 拓展名 是合并存储的, 所以我们提供转换功能 , 此处还是使用宏来偷懒... */
#define _(m, Ptr)                                    \
    for (int i = 0; i < sizeof(m) / sizeof(*m); i++) \
        if (m[i] == 0xFFFF)                          \
            break;                                   \
        else                                         \
            *Ptr++ = m[i] & 0xFF;                    \

static inline char *_CharNamel (EntryLong_t *Long, char *Buffer)
{
    char *Ptr = Buffer;

    _(Long->Name1, Ptr);
    _(Long->Name2, Ptr);
    _(Long->Name3, Ptr);

    *Ptr = '\0';
    return Buffer;
}

#undef _

static inline char *_CharName (Entry_t *Entry, char *Buffer)
{
    char *Ptr = Buffer;
    char *Name = Entry->Name;

    for (int i = 0; i < _LEN_NAME && Name[i] != ' '; i++)
        *Ptr++ = Name[i];
    for (int i = 0; i < _LEN_EXT && Name[i + 8] != ' '; i++)
        *Ptr++ = Name[i + 8];

    *Ptr = '\0';
    return Buffer;
}

/*
    路径游走 , 我第一次听到这个好听的名字是在 lunaixsky 的视频里面, 这里直接沿用了!

    @param Parent  父目录
    @param Target  搜索对象文件名
    @param ReadDir 控制是否是读取目录, 即是否对每个 Entry 都创建一个 Node
*/
static Node_t *_SearchEntry (Node_t *Parent, char *Target, bool ReadDir)
{
    Info_t *Sys = Parent->Private.Sys;

    Node_t *Res = NULL;

    for (size_t SectorIdx = 0 ;  ; SectorIdx++) {
        void *Sector = MallocK(Sys->Record->SecSiz);
        Sys->Dev->BlkRead (Parent->Private.Addr + SectorIdx, Sector, 1);

        for (size_t EntryIdx = 0 ; EntryIdx < Sys->Record->SecSiz / sizeof(Entry_t) ; EntryIdx++) {
            Entry_t *Entry = (Entry_t *)Sector + EntryIdx;
            Entry_t *Item  = NULL;

            if (*(u64 *)Entry == 0)
                goto OptEnd;

            char Buffer[64];

            if (Entry->Attr == FA_LONG) {
                _CharNamel ((EntryLong_t *)Entry, Buffer);
                Item = _EntryDup(Entry + 1); // TODO: 目录项处于下一个扇区
                
                EntryIdx++;
            } else {
                _CharName (Entry, Buffer);
                Item = _EntryDup(Entry);
            }

            bool Hit = _CmpBuf (Target, Buffer);
            if (Item && (Hit || ReadDir)) {
                Node_t *Node = MallocK(sizeof(Node_t));
                memset (Node, 0, sizeof(Node_t));

                Node->Name = strdup (Buffer);
                if (Item->Attr & FA_DIR)
                    Node->Attr |= NA_DIR;
                else if (Item->Attr & FA_ARCHIVE)
                    Node->Attr |= NA_ARCHIVE;
                Node->Siz = Item->FileSiz;

                Node->Root = Parent->Root;
                Node->Private.Sys = Sys;
                Node->Private.Addr = _AddrGet (Sys, Item);                

                Node->Parent = Parent;
                Node->Child = NULL;

                Node->Opts = Node->Root->Opts;

                if (Parent->Child == NULL)
                    Parent->Child = Node;
                else {
                    Node->Next = Parent->Child->Next;
                    Parent->Child->Next = Node;
                }

                Res = Node;

                if (Hit) goto OptEnd;
            }

            if (Item)
                FreeK(Item);
        }
        goto SecNxt;
OptEnd:
        FreeK (Sector);
        break;
SecNxt:
        FreeK (Sector);
    }

    return ReadDir ? Parent : Res;
}

static Node_t *Fat32_PathWalk (Node_t *Start, char *Path)
{
    /* Preparation ends here */

    u64 Addr = Start->Private.Addr;
    Node_t *Prev = Start;

    Node_t *Res = NULL;

    for (;;)
    {
        char *Nxt = _Next(Path);

        Node_t *Read = _SearchEntry (Prev, Path, false);
        if (!Read)
            return NULL;

        Addr = Read->Private.Addr;

        if (Nxt[0] == 0) {
            Res = Read;
            break;
        }

        Path = Nxt;
        Prev = Read;
    }

    if (Res->Attr & NA_DIR)
        Res = _SearchEntry (Prev, "", true);

    return Res;
}
