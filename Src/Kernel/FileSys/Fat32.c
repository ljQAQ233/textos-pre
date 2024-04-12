#include "Base.h"
#include "Types.h"

#include <string.h>

#define FAT_EOF 0x0FFFFFFF          // EOF in FAT1

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

    u64           FatSec;
    u64           FirstDataSec;
    Record_t      *Mbs;       // The master boot sector of this partition
    Fat32Record_t *Record;    // The boot record of this Fat32 file system
    Partition_t   *Partition; // The partition entry in Mbs
} Info_t;

#include <TextOS/Dev/Ide.h>
#include <TextOS/Memory/Malloc.h>

#include <TextOS/Debug.h>
#include <TextOS/Console/PrintK.h>

static Node_t *Fat32_PathWalk (Node_t *Start, char *Path);

static Node_t *Fat32_Open  (Node_t *This, char *Path, u64 Args);
static int     Fat32_Close (Node_t *This);
static int     Fat32_Read  (Node_t *This, void *Buffer, size_t Siz, size_t Offset);

static Node_t *_SearchEntry (Node_t *Parent, char *Target, bool ReadDir);

FsOpts_t __Fat32_Opts = {
    .Open = Fat32_Open,
    .Read = Fat32_Read,
    .Close = Fat32_Close,
};

FS_INITIALIZER(__FsInit_Fat32)
{
    Fat32Record_t *Record = MallocK(sizeof(Fat32Record_t));
    Hd->BlkRead (Hd, Partition->Relative, Record, 1);

    if (Record->EndSym != 0xAA55)
        return NULL;

    u64 FatSiz = Record->FatSiz16 == 0 ? Record->FatSiz32 : Record->FatSiz16;

    u64 FatTabSec    = Partition->Relative + Record->SecReserved;
    u64 FirstDataSec = FatTabSec
                     + Record->FatNum * FatSiz
                     + (Record->Roots * 32 + (Record->SecSiz - 1)) / Record->SecSiz; // root_dir_sectors

    u64 Root = (Record->RootClus - 2) * Record->SecPerCluster + FirstDataSec;
    DEBUGK ("Fat32 -> Tab : %#x (%u,%u) , First data sector : %#x , Root : %#x {%d}\n", FatTabSec, Record->FatNum, FatSiz, FirstDataSec, Root, Record->SecPerCluster);

    Info_t *Sys = MallocK(sizeof(Info_t));
    Node_t *Ori = MallocK(sizeof(Node_t));

    /* 记录文件系统信息 */
    Sys->Dev = Hd;
    Sys->Mbs = Mbs;
    Sys->Record = Record;
    Sys->Partition = Partition;
    Sys->FatSec = FatTabSec;
    Sys->FirstDataSec = FirstDataSec;

    /* 初始化此文件系统根节点 */
    Ori->Attr = NA_DIR | NA_PROTECT;
    Ori->Name = "/";
    Ori->Root = Ori;
    Ori->Child = NULL;
    Ori->Parent = Ori;
    Ori->Private.Sys = Sys;
    Ori->Private.Addr = Root;

    Ori->Siz = 0;

    /* FAT32 文件系统接口 */
    Ori->Opts = &__Fat32_Opts;

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
    FreeK (This);

    return 0;
}

/* 已经处于一个尴尬的境地了 -> 因为之前已经将 目录 读取进 `_Root` */
static int _ReadDir(Node_t *This, void *Buffer, size_t Count)
{
    int Real = 0;

    for (Node_t *p = This->Child; p != NULL; p = p->Next)
        Real++;

    return Real;
}

static int _ReadContent (Node_t *This, void *Buffer, size_t ReadSiz, size_t Offset)
{
    Info_t *Sys = This->Private.Sys;

    if (ReadSiz == 0)
        return 0;
    if (Offset >= This->Siz)
        return EOF;
    
    int Real = MIN(This->Siz, Offset + ReadSiz) - Offset;
    size_t Count = DIV_ROUND_UP(Real, Sys->Record->SecSiz);

    /* TODO: Replace it */
    void *Tmp = MallocK(Sys->Record->SecSiz * Count);
    Sys->Dev->BlkRead (
        Sys->Dev,
        This->Private.Addr,
        Tmp, Count
    );
    memcpy (Buffer, Tmp + (Offset % SECT_SIZ), Real);

    FreeK(Tmp);

    return Real;
}

static int Fat32_Read (Node_t *This, void *Buffer, size_t Siz, size_t Offset)
{
    if (This->Attr & NA_ARCHIVE)
        return _ReadContent (This, Buffer, Siz, Offset);

    return _ReadDir (This, NULL, Siz);   // TODO
}

#define _LEN_NAME 8
#define _LEN_EXT 3

#include <string.h>

_UTIL_NEXT();

static Entry_t *_EntryDuplicate (Entry_t *Ori)
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

static bool _MatchName(char *A, char *B)
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
#define _(m, c, fini, Ptr)                                    \
    for (int i = 0; i < sizeof(m) / sizeof(*m); i++, (c)++) { \
        if (m[i] == 0xFFFF || *Ptr == '/')                    \
            goto fini;                                        \
        if (m[i] != *Ptr++)                                   \
            return false;                                     \
    }                                                         \

static inline bool _MatchNamel (EntryLong_t *Long, char **Name, size_t *Siz)
{
    char *Ptr = *Name;

    _(Long->Name1, *Siz, fini, Ptr);
    _(Long->Name2, *Siz, fini, Ptr);
    _(Long->Name3, *Siz, fini, Ptr);

fini:
    *Name = Ptr;

    return (*Ptr == 0 || *Ptr == '/') ? true : false;
}

#undef _

#define _(m, Ptr)                                     \
    for (int i = 0; i < sizeof(m) / sizeof(*m); i++)  \
        if (m[i] == 0xFFFF)                           \
            break;                                    \
        else                                          \
            *Ptr++ = m[i];                            \

static inline char *_ParseNamel (EntryLong_t *Long, char **Buffer)
{
    char *Ptr = *Buffer;

    _(Long->Name1, Ptr);
    _(Long->Name2, Ptr);
    _(Long->Name3, Ptr);

    return *Buffer = Ptr;
}

#undef _

static inline char *_ParseName (Entry_t *Entry, char *Buffer)
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

//

#define DUMP_IC(Sys, Sect)    ((Sect - Sys->FirstDataSec) / Sys->Record->SecPerCluster + 2)
#define DUMP_IS(Sys, Cluster) ((Cluster - 2) * Sys->Record->SecPerCluster + Sys->FirstDataSec)

/*
    路径游走 , 我第一次听到这个好听的名字是在 lunaixsky 的视频里面, 这里直接沿用了!

    根据 Parent 指定的 Addr 遍历 FAT1 中的对应项, 直到遇到 FAT_EOF 为止

    @param Parent  父目录
    @param Target  搜索对象文件名
    @param ReadDir 控制是否是读取目录, 即是否对每个 Entry 都创建一个 Node
*/
#define FAT_ALOC_NR  (SECT_SIZ / sizeof(u32))
#define FAT_ENTRY_NR (SECT_SIZ / sizeof(Entry_t))

#define ALIGN_DOWN(Target, Base) ((Base) * (Target / Base))

static Node_t *_SearchEntry (Node_t *Parent, char *Target, bool ReadDir)
{
    Info_t *Sys = Parent->Private.Sys;

    u32 *Idxes = MallocK (SECT_SIZ);
    u32 Curr = DUMP_IC(Sys, Parent->Private.Addr);
    Sys->Dev->BlkRead (
        Sys->Dev,
        Sys->FatSec + (Curr * 4) / SECT_SIZ,
        Idxes, 1
    );

    char *Name;
    Node_t *Child;
    
    while (true)
    {
        Entry_t *Template = NULL;

        Entry_t *Entries = MallocK (SECT_SIZ);
        Sys->Dev->BlkRead (Sys->Dev, DUMP_IS(Sys, Curr), Entries, 1);

        for (int i = 0 ; i < SECT_SIZ / sizeof(Entry_t) ; i++)
        {
            if (Entries[i].Attr & FA_LONG)
            {
                char *Ptr = Target;
                size_t Siz = 0;
                EntryLong_t *Cmp = (EntryLong_t *)&Entries[i];
                for (int j = i ; j < SECT_SIZ / sizeof(Entry_t) ; j++)
                {
                    if (!_MatchNamel (Cmp, &Ptr, &Siz)) {
                        i = j + 1;
                        goto EntryNxt;
                    }
                    if (*Ptr == 0 || *Ptr == '/') {
                        Name = MallocK (Siz);
                        char *Ptr = Name;
                        while (i <= j)
                            _ParseNamel ((EntryLong_t *)&Entries[i++], &Ptr);
                        Template = _EntryDuplicate (&Entries[j+1]);
                        goto NodeCreate;
                    }
                    Cmp++;
                }

                goto NodeCreate;
            }

            /* If it is not a long entry, then dump its name first and compare it with `Target` */
            char Tmp[16] = "";
            if (_MatchName (_ParseName (&Entries[i], Tmp), Target))
            {
                Name = strdup (Tmp);
                Template = _EntryDuplicate (&Entries[i]);
                goto NodeCreate;
            }

        /* Handle the next entry we have read before */
        EntryNxt:
            continue;
        }

        /*
           OK , all of the current entries have been handled,
           if the next cluster index is in `Idxes`, do nothing,
           otherwise -> Read the sector which the index locates in
        */

        /* Reaches the end */
        if (Idxes[Curr % FAT_ALOC_NR] == FAT_EOF)
            break;
        if (!(ALIGN_DOWN(Curr, FAT_ALOC_NR) <= Idxes[Curr % FAT_ALOC_NR]
              && Idxes[Curr % FAT_ALOC_NR] < ALIGN_DOWN(Curr, FAT_ALOC_NR) + FAT_ALOC_NR))
        {
            u32 New = Idxes[Curr % FAT_ALOC_NR];
            Sys->Dev->BlkRead (
                Sys->Dev,
                Sys->FatSec + (Idxes[Curr] * sizeof(u32)) / SECT_SIZ,
                Idxes, 1
            );
            Curr = New;
        }

    NodeCreate:
        /* Basic info of node */
        Child = MallocK(sizeof(Node_t));
        Child->Name = Name;
        Child->Siz = Template->FileSiz;
        Child->Parent = Parent;
        Child->Opts = Parent->Opts;
        Child->Private.Sys = Parent->Private.Sys;
        Child->Private.Addr = _AddrGet (Sys, Template);
        Child->Attr = 0;
        if (Template->Attr & FA_ARCHIVE)
            Child->Attr |= NA_ARCHIVE;
        else
            Child->Attr |= NA_DIR;

        /* Update parent node */
        Child->Next = Parent->Child;
        Parent->Child = Child;

        break;
    }

    return Child;
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
