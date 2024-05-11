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

#define LEN_NAME 8
#define LEN_EXT  3

typedef struct _packed
{
    u8  Order;
    u16 Name1[5];
    u8  Attr;
    u8  Type;    // Always zero
    u8  ShortCkSum;
    u16 Name2[6];
    u16 Cluster; // Always zero
    u16 Name3[2];
} EntryLong_t;

#define _LEN_LONGW (13 * sizeof(u16))
#define _LEN_LONGC (13 * sizeof(u8))

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

#include <TextOS/Lib/List.h>
#include <TextOS/Lib/Stack.h>

#include <TextOS/Dev/Ide.h>
#include <TextOS/Memory/Malloc.h>

#include <TextOS/ErrNo.h>
#include <TextOS/Debug.h>
#include <TextOS/Panic.h>
#include <TextOS/Assert.h>

static int Fat32_Open (Node_t *Parent, char *Path, u64 Args, Node_t **Result);
static int Fat32_Close (Node_t *This);
static int Fat32_Remove (Node_t *This);
static int Fat32_Read (Node_t *This, void *Buffer, size_t Siz, size_t Offset);
static int Fat32_Write (Node_t *This, void *Buffer, size_t Siz, size_t Offset);
static int Fat32_Truncate (Node_t *This, size_t Offset);
static int Fat32_ReadDir (Node_t *This);

FsOpts_t __Fat32_Opts = {
    .Open = Fat32_Open,
    .Close = Fat32_Close,
    .Remove = Fat32_Remove,
    .Read = Fat32_Read,
    .Write = Fat32_Write,
    .Truncate = Fat32_Truncate,
    .ReadDir = Fat32_ReadDir,
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
    Ori->Private.SysType = FS_FAT32;
    Ori->Private.Addr = Root;

    Ori->Siz = 0;

    /* FAT32 文件系统接口 */
    Ori->Opts = &__Fat32_Opts;

    return Ori;
}

//
// 好了, 下面是 entry 操作
//

#define ALIGN_UP(Target, Base) ((Base) * ((Target + Base - 1) / Base))
#define ALIGN_DOWN(Target, Base) ((Base) * (Target / Base))

#define DUMP_IC(Sys, Sect)    (((Sect) - (Sys)->FirstDataSec) / (Sys)->Record->SecPerCluster + 2)
#define DUMP_IS(Sys, Cluster) (((Cluster) - 2) * (Sys)->Record->SecPerCluster + (Sys)->FirstDataSec)

/* 获取此 Cluster 处于 FAT1 中的地址 */
#define TAB_IS(Sys, Cluster) (Sys->FatSec + (Cluster * 4) / SECT_SIZ)

#define FAT_ALOC_NR  (SECT_SIZ / sizeof(u32))     // 一扇区 FAT 表的索引容量
#define FAT_ENTRY_NR (SECT_SIZ / sizeof(Entry_t)) // 一扇区 Entry 容量

typedef struct
{
    u32     Cluster;
    u64     Idx;
    List_t  List;
} Locate_t;

/* ls : a ptr to a list head */
#define LOCATOR_ADD(ls, c, i)                         \
        do {                                          \
            Locate_t *l = MallocK (sizeof(Locate_t)); \
            l->Cluster = c;                           \
            l->Idx = i;                               \
            ListInsert ((ls), &l->List);              \
        } while (false);

#define LOCATOR_CLR(ls)                           \
        do {                                      \
            Locate_t *l;                          \
            while (!ListEmpty (ls)) {             \
                l = CR((ls)->Back, Locate_t, List); \
                FreeK (l);                        \
                ListRemove ((ls)->Back);            \
            }                                     \
        } while (false);                          \

/* 这里使用一种抽象是为了缩小代码体积, 以及简化一些重复操作 */
typedef struct
{
    Info_t  *Sys;         // 文件系统信息
    struct {              // 存储着一些位置信息
        u32    Cluster;   // 本结构所代表的 entry 的簇号
                          // 如果是 dir_entry , 是索引此目录下文件的那几个簇的起始簇号
                          // 如果是 archive_entry , 是此文件内容的起始簇簇号
        List_t List;      // 描述各个 entry 位于哪个簇, 是第几个 entry
    } Locator;
    Stack_t Stack;        // entry 栈
    Node_t  *Node;        // entry 被解析后生成的 vfs node
} Lookup_t;

static inline Lookup_t *_LkpOri (Node_t *Node, Lookup_t *Lkp)
{
    if (!Lkp)
        Lkp = MallocK (sizeof(Lookup_t));

    Lkp->Locator.Cluster = DUMP_IC(((Info_t *)Node->Private.Sys), Node->Private.Addr);
    StackInit (&Lkp->Stack);
    ListInit  (&Lkp->Locator.List);
    Lkp->Node = Node;
    Lkp->Sys = Node->Private.Sys;

    return Lkp;
}

#define LKP_ORI(n) \
        (_LkpOri (n, NULL))

static size_t _AllocPart (Info_t *Sys);
static size_t _Expand (Node_t *This, size_t Siz);
static size_t _ExpandPart (Node_t *This, size_t Cnt, bool Append);

/*
    在一个父 Entry 下查找子项, 并生成 Lkp, 包含信息 (行为) 如下:
      - 一个栈
        - 保存着 entry 信息, 包含长短目录项
      - 一个 Locator 定位子
        - 一个 描述当前 entry 所指向的 文件/目录 的簇号 (Cluster)
        - 一个 描述 entry 在文件系统中的具体位置的链表  (List)
      - vfs 抽象的子节点
          - 只是在物理文件系统层面的初始化 -> 不会参与 节点 有关虚拟文件系统部分的初始化
      - vfs 环境 (所处的文件系统信息) -> Sys
*/
static Lookup_t *_LookupEntry (Lookup_t *Parent, char *Target, size_t Idx);

enum {
    LKPS_NONE   = 0x00,
    LKPS_UPDATE = 0x01,
    LKPS_CREATE = 0x02,
    LKPS_CHILD  = 0x04,
    LKPS_ERASE  = 0x08
};

static void      _LookupSave (Lookup_t *Lkp, Stack_t *Update, int Opt);

//

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

// static inline size_t _ValidChr (const char *Str)
// {
//     size_t i = 0;
//     while (Str[i]) {
//         if (Str[i] == ' ' || Str[i] == '/')
//             break;
//         i++;
//     }

//     return i;
// }

// static bool _MatchName (char *A, char *B)
// {
//     size_t LenA = _ValidChr(A),
//            LenB = _ValidChr(B);
//     if (LenA != LenB)
//         return false;

//     for (int i = 0; i < LenB; i++)
//         if (A[i] != B[i])
//             return false;

//     return true;
// }

// /* 长目录项的 文件名 以及 拓展名 是合并存储的, 所以我们提供转换功能 , 此处还是使用宏来偷懒... */
// #define _(m, c, fini, Ptr)                                    \
//     for (int i = 0; i < sizeof(m) / sizeof(*m); i++, (c)++) { \
//         if (m[i] == 0xFFFF || *Ptr == '/' || *Ptr == 0)       \
//             goto fini;                                        \
//         if (m[i] != *Ptr++)                                   \
//             return false;                                     \
//     }                                                         \

// static inline bool _MatchNamel (EntryLong_t *Long, char **Name, size_t *Siz)
// {
//     char *Ptr = *Name;

//     _(Long->Name1, *Siz, fini, Ptr);
//     _(Long->Name2, *Siz, fini, Ptr);
//     _(Long->Name3, *Siz, fini, Ptr);

// fini:
//     *Name = Ptr;

//     return (*Ptr == 0 || *Ptr == '/') ? true : false;
// }

// #undef _

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

    for (int i = 0; i < LEN_NAME && Name[i] != ' '; i++)
        *Ptr++ = Name[i];
    for (int i = 0; i < LEN_EXT && Name[i + 8] != ' '; i++)
        *Ptr++ = Name[i + 8];

    *Ptr = '\0';
    return Buffer;
}

#define _(m, Ptr)                                      \
    for (int i = 0 ; i < sizeof(m) / sizeof(*m) ; i++) \
            m[i] = *Ptr++;                             \

static inline char *_MakeNamel (EntryLong_t *Long, char **Buffer)
{
    char *Ptr = *Buffer;

    u16 Tmp[_LEN_LONGW];
    for (int i = 0 ; i < _LEN_LONGW ; i++) {
        if (*Ptr == 0) {
            Tmp[i] = 0x0000;
            for (i = i + 1 ; i < _LEN_LONGW ; i++ )
                Tmp[i] = 0xFFFF;
            break;
        }
        Tmp[i] = *Ptr++;
    }

    do {
        u16 *Cpyr = Tmp;
        _(Long->Name1, Cpyr);
        _(Long->Name2, Cpyr);
        _(Long->Name3, Cpyr);
    } while (false);

    return *Buffer = Ptr;
}

#undef _

#define ENTRY_VALID(Entry) ((Entry).Name[0] != 0 && (u8)(Entry).Name[0] != 0xe5)
#define ENTRY_ERASE(Entry) ((Entry).Name[0] =  0)

static void _DestroyHandler (void *Payload)
{
    FreeK (Payload);
}

static inline u8 _CkSum (char *Short) 
{ 
    u8 Sum = 0;
    u8 *Ptr = (u8 *)Short;
    
    for (short Len = 11 ; Len!=0 ; Len--) {
        // NOTE: The operation is an unsigned char rotate right
        Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + *Ptr++;
    }
    return Sum; 
} 

#define ORDER_LAST 0x40

/* 调用函数来完成也许是无必要的, 即使编译器会优化 */
#define UPPER_CASE(c) ('a' <= c && c <= 'z' ? c - 32 : c)
#define LOWER_CASE(c) ('A' <= c && c <= 'Z' ? c + 32 : c)

static Stack_t *_MakeEntry (Node_t *Target)
{
    Stack_t *Stack = StackInit (NULL);
    ASSERTK (Stack != NULL);
    
    bool Longer = false;

    char *Name = strdup (Target->Name);    
    /* 预处理 -> 去除末尾的 `.` */
    for (int i = strlen(Name) - 1 ; i >= 0 ; i--) {
        if (Name[i] == '.')
            Name[i] = EOS;
        else
            break;
    }

    /* TODO: replace it -> 使用总名称长度来判断是完全错误的, 应该分开! */
    int Len = strlen (Name);
    if (Len > LEN_EXT + LEN_NAME)
    {
        Longer = true;
        goto Make;
    }

    /*
       NOTE: All the characters which are not supported by short entry will be replaced by '_'
    */
    size_t NrDots = 0;
    for (int i = 0 ; i < Len ; i++) {
        switch (Name[i]) {
            /* Unsupported chars */
            case '+': case ',': case ';':
            case '=': case '[': case ']':
                Name[i] = '_';
            
            case ' ':
            case 'a'...'z':
                Longer = true;
                goto Make;
            
            case '.':
                if (++NrDots > 1)
                    Longer = true;
            break;

            default: break;
        }
    }
    
    //

    int ExtStart, NameStart;
    char _Name[LEN_NAME],
         _Ext [LEN_EXT ];
    int  _Namei,
         _Exti;
Make:
    memset (_Name, ' ', sizeof(_Name));
    memset (_Ext , ' ', sizeof(_Ext ));
    /* 对于 `.XXXX` 的 情况, `.XXXX` 不算做拓展名 */
    ExtStart = 0;
    NameStart = 0;
    for (int i = Len - 1 ; i >= 0 ; i--) {
        if (Name[i] == '.')
            ExtStart = i+1;
    }
    for (int i = 0 ; i < Len ; i++) {
        if (Name[i] == '.')
            continue;

        NameStart = i;
        break;
    }

    /* 开始填充 */
    _Exti = 0;
    _Namei = 0;
    if (ExtStart != 0) {
        for (int i = ExtStart ; i < Len ; i++) {
            char Append = 0;
            switch (Name[i]) {
                case ' ':
                    continue;

                default:
                    Append = UPPER_CASE(Name[i]);
                break;
            }

            if (_Exti < LEN_NAME)
                _Ext[_Exti++] = Append;
        }
    } else {
        /* 不阻挡 Name 的初始化 */
        ExtStart = 0xff;
    }

    for (int i = NameStart ; i < Len && i < ExtStart ; i++) {
        char Append = 0;
        switch (Name[i]) {
            case '.':
            case ' ':
                continue;

            default:
                Append = UPPER_CASE(Name[i]);
            break;
        }

        if (_Namei < LEN_NAME)
            _Name[_Namei++] = Append;
    }
    
    FreeK (Name);

    Info_t *Sys = Target->Private.Sys;

    Entry_t *Short = MallocK (sizeof(Entry_t));
    memset (Short, 0, sizeof(Entry_t));
    memcpy (Short->Name, _Name, LEN_NAME);
    memcpy (Short->Name + LEN_NAME, _Ext, LEN_EXT);
    Short->FileSiz = Target->Siz;
    if (Target->Private.Addr) {
        Short->ClusterHigh = DUMP_IC(Sys, Target->Private.Addr) >> 16;
        Short->ClusterLow  = DUMP_IC(Sys, Target->Private.Addr) &  0xFFFF;
    }
    Short->Attr = (Target->Attr & NA_ARCHIVE ? FA_ARCHIVE : FA_DIR);
    StackPush (Stack, Short);

    /* 接下来是长目录的主场 (有的话...) */

    u8 CkSum = _CkSum (Short->Name);
    if (Longer) {
        int Cnt = DIV_ROUND_UP(Len, _LEN_LONGW);

        char *Ptr = Target->Name;
        EntryLong_t *Long;
        for (int i = 0 ; i < Cnt ; i++)
        {
            Long = MallocK (sizeof(EntryLong_t));
            memset (Long, 0, sizeof(EntryLong_t));
            memset (Long->Name1, 0xFF, sizeof(Long->Name1));
            memset (Long->Name2, 0xFF, sizeof(Long->Name2));
            memset (Long->Name3, 0xFF, sizeof(Long->Name3));

            _MakeNamel (Long, &Ptr);

            // TODO : Check sum
            Long->Attr = FA_LONG;
            Long->Order = i + 1;
            Long->ShortCkSum = CkSum;
            
            StackPush (Stack, Long);
        }

        /* Set mask for the last entry */
        Long->Order |= ORDER_LAST;
    }

    return Stack;
}

static Node_t *_AnalysisEntry (Info_t *Sys, Stack_t *Stack)
{
    ASSERTK (!StackEmpty (Stack));
    StackIter_t *Iter = StackIter (Stack);

    Node_t *Node = MallocK (sizeof(Node_t));
    memset (Node, 0, sizeof(Node_t));

    Entry_t *Main = StackIterDef (Iter);
                    StackIterNext (&Iter);

    Node->Siz = Main->FileSiz;
    Node->Attr = 0;
    if (Main->Attr & FA_DIR)
        Node->Attr |= NA_DIR;
    else if (Main->Attr & FA_ARCHIVE)
        Node->Attr |= NA_ARCHIVE;

    if (Main->ClusterLow || Main->ClusterHigh)
        Node->Private.Addr = _AddrGet (Sys, Main);

    size_t Cnt = StackSiz (Stack) - 1;
    /* Only has a main entry (short entry) */
    if (Cnt == 0)
    {
        /* Allocate memory for it in stack in order to improve the speed */
        char Buffer[LEN_NAME + LEN_EXT];
        memset (Buffer, 0, sizeof(Buffer));
        /* Do parsing & copying... */
        Node->Name = strdup (_ParseName (Main, Buffer));
    }
    else
    {
        Node->Name = MallocK (_LEN_LONGC * Cnt);
        
        do {
            char *Ptr = Node->Name;
            while (Cnt--)
            {
                _ParseNamel (StackIterDef (Iter), &Ptr);
                             StackIterNext (&Iter);
            }
        } while (false);
    }

    return Node;
}

/* 根据 Stack 来申请, 结果保存到 Lkp 的 List 中 */
static void _LookupAlloc (Lookup_t *Parent, Lookup_t *Lkp, Stack_t *Stack)
{
    Info_t *Sys = Parent->Sys;

    u32 *Idxes = MallocK (SECT_SIZ);
    u32 Curr = Parent->Locator.Cluster;
    u32 Prev = Parent->Locator.Cluster;
    Sys->Dev->BlkRead (Sys->Dev, TAB_IS(Sys, Curr), Idxes, 1);

    size_t StartIdx = 0, CurrIdx  = 0;

    size_t Cnt = StackSiz (Stack);
    StackInit (&Lkp->Stack);
    ListInit  (&Lkp->Locator.List);
    
    bool Hit = false;
    while (true)
    {
        Entry_t *Entries = MallocK (SECT_SIZ);
        Sys->Dev->BlkRead (Sys->Dev, DUMP_IS(Sys, Curr), Entries, 1);

        for (int i = 0 ; i < FAT_ENTRY_NR ; i++, CurrIdx++)
        {
            /* 希望它 free */
            if (ENTRY_VALID(Entries[i])) {
                Prev = Curr;
                StartIdx = CurrIdx + 1;
                LOCATOR_CLR(&Lkp->Locator.List);
                continue;
            }

            LOCATOR_ADD(&Lkp->Locator.List, Curr, i);
            if (CurrIdx - StartIdx + 1 == Cnt) {
                Hit = true;
                goto End;
            }
        }

        /* Reaches the end */
        if (Idxes[Curr % FAT_ALOC_NR] == FAT_EOF)
            goto End;
        if (!(ALIGN_DOWN(Curr, FAT_ALOC_NR) <= Idxes[Curr % FAT_ALOC_NR]
              && Idxes[Curr % FAT_ALOC_NR] < ALIGN_DOWN(Curr, FAT_ALOC_NR) + FAT_ALOC_NR))
        {
            u32 New = Idxes[Curr % FAT_ALOC_NR];
            Sys->Dev->BlkRead (
                Sys->Dev,
                TAB_IS(Sys, Idxes[Curr]),
                Idxes, 1
            );
            Prev = Curr;
            Curr = New;
        } else {
            Prev = Curr;
            Curr = Idxes[Curr % FAT_ALOC_NR];
        }

        continue;

    End:
        FreeK (Entries);
        break;
    }
    FreeK (Idxes);
    
    if (!Hit) {
        _ExpandPart (Parent->Node, 1, true);
        _LookupAlloc (Parent, Lkp, &Lkp->Stack);
    }
}

static void _LookupRelease (Lookup_t *Lookup)
{
    if (!StackEmpty (&Lookup->Stack))
        StackClear (&Lookup->Stack);

    /* TODO: 可行性检查 -> 是否需要释放 */
    if (Lookup->Node)
        __VrtFs_Release (Lookup->Node);
}

static Lookup_t *_LookupEntry (Lookup_t *Parent, char *Target, size_t Idx)
{
    Lookup_t *Lookup = MallocK (sizeof(Lookup_t));

    StackInit (&Lookup->Stack);
    StackSet  (&Lookup->Stack, _DestroyHandler, _DestroyHandler);
    ListInit  (&Lookup->Locator.List);
    
    Info_t *Sys = Parent->Sys;
    u32 Curr = Parent->Locator.Cluster;
    u32 *Idxes = MallocK (SECT_SIZ);
    Sys->Dev->BlkRead (Sys->Dev, TAB_IS(Sys, Curr), Idxes, 1);

    while (true)
    {
        Entry_t *Entries = MallocK (SECT_SIZ);
        Sys->Dev->BlkRead (Sys->Dev, DUMP_IS(Sys, Curr), Entries, 1);

        for (int i = 0 ; i < SECT_SIZ / sizeof(Entry_t) ; i++)
        {
            /* 这个文件已经被删除了 (不存在) */
            if (!ENTRY_VALID(Entries[i]))
                goto EntryNxt;

            LOCATOR_ADD (&Lookup->Locator.List, Curr, i);
            if (Entries[i].Attr & FA_LONG)
            {
                EntryLong_t *Long, *Prev;

                Long = (void *)&Entries[i];
                if (~Long->Order & ORDER_LAST)
                    goto EntryNxt;

                Prev = StackTop(&Lookup->Stack);
                ASSERTK (Prev ? Long->ShortCkSum == Prev->ShortCkSum : true);
                StackPush (&Lookup->Stack, _EntryDuplicate (&Entries[i]));

                goto EntryNxt;
            }

            if (!StackEmpty (&Lookup->Stack)) {
                EntryLong_t *Prev = StackTop (&Lookup->Stack);

                u8 CkSum = _CkSum (Entries[i].Name);
                if (Prev->ShortCkSum != CkSum) {
                    /* Starts from the origin */
                    StackClear (&Lookup->Stack);
                    goto EntryNxt;
                }
            }

            /* store the short entry so that we can handle
               it and its attached entries at the same time */
            StackPush (&Lookup->Stack, &Entries[i]);

            Lookup->Node = _AnalysisEntry (Sys, &Lookup->Stack);
            if (Target == NULL) {
                if (Idx-- == 0)
                    goto Brk;
                goto LkpRst;
            }

            size_t CmpSiz = (size_t)(strchrnul (Target, '/') - Target);
            if (strncmp (Target, Lookup->Node->Name, CmpSiz) == 0)
                goto Brk;

        LkpRst:
            __VrtFs_Release (Lookup->Node);
            Lookup->Node = NULL;
            StackClear (&Lookup->Stack);
            LOCATOR_CLR(&Lookup->Locator.List);

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
            goto Brk;
        if (!(ALIGN_DOWN(Curr, FAT_ALOC_NR) <= Idxes[Curr % FAT_ALOC_NR]
              && Idxes[Curr % FAT_ALOC_NR] < ALIGN_DOWN(Curr, FAT_ALOC_NR) + FAT_ALOC_NR))
        {
            u32 New = Idxes[Curr % FAT_ALOC_NR];
            Sys->Dev->BlkRead (
                Sys->Dev,
                TAB_IS(Sys, Idxes[Curr]),
                Idxes, 1
            );
            Curr = New;
        } else {
            Curr = Idxes[Curr % FAT_ALOC_NR];
        }

        continue;
    
    Brk:
        FreeK (Entries);
        break;
    }

    FreeK (Idxes);
    if (!Lookup->Node) {
        FreeK (Lookup);
        Lookup = NULL;
    } else {
        Lookup->Sys = Parent->Sys;
        // Lookup->Locator.Cluster = DUMP_IC(Sys, Lookup->Node->Private.Addr);
    }
    return Lookup;
}

static void _LookupAll (Lookup_t *Parent, Stack_t *Child)
{
    Info_t *Sys = Parent->Sys;
    u32 Curr = Parent->Locator.Cluster;
    u32 *Idxes = MallocK (SECT_SIZ);
    Sys->Dev->BlkRead (Sys->Dev, TAB_IS(Sys, Curr), Idxes, 1);

    while (true)
    {
        Entry_t *Entries = MallocK (SECT_SIZ);
        Sys->Dev->BlkRead (Sys->Dev, DUMP_IS(Sys, Curr), Entries, 1);

        for (int i = 0 ; i < SECT_SIZ / sizeof(Entry_t) ; i++)
        {
            /* 这个文件已经被删除了 (不存在) */
            if (!ENTRY_VALID(Entries[i]))
                goto EntryNxt;
            StackPush (Child, _EntryDuplicate(&Entries[i]));

        EntryNxt:
            continue;
        }

        if (Idxes[Curr % FAT_ALOC_NR] == FAT_EOF)
            goto Brk;
        if (!(ALIGN_DOWN(Curr, FAT_ALOC_NR) <= Idxes[Curr % FAT_ALOC_NR]
              && Idxes[Curr % FAT_ALOC_NR] < ALIGN_DOWN(Curr, FAT_ALOC_NR) + FAT_ALOC_NR))
        {
            u32 New = Idxes[Curr % FAT_ALOC_NR];
            Sys->Dev->BlkRead (Sys->Dev, TAB_IS(Sys, Idxes[Curr]), Idxes, 1);
            Curr = New;
        } else {
            Curr = Idxes[Curr % FAT_ALOC_NR];
        }

        continue;
    Brk:
        FreeK (Entries);
        break;
    }

    FreeK (Idxes);
}

/* Write a stack `Update` according to the list in `Origin` */
static void _LookupSave_Common (Lookup_t *Origin, Stack_t *Update)
{
    Info_t *Sys = Origin->Sys;

    u32 Prev = 0;
    List_t *Head = &Origin->Locator.List;

    /* 逆向遍历, 顺应 Stack */
    Entry_t *Entries = MallocK (SECT_SIZ);
    for (List_t *Ptr = Head->Forward ; Ptr != Head ; Ptr = Ptr->Forward) {
        Locate_t *Locator = CR (Ptr, Locate_t, List);
        if (Prev != Locator->Cluster) {
            /* 准备进入下一个扇区, 先保存 */
            if (Prev) {
                Sys->Dev->BlkWrite (Sys->Dev, DUMP_IS(Sys, Prev), Entries, 1);
            }
            Prev = Locator->Cluster;
            Sys->Dev->BlkRead (Sys->Dev, DUMP_IS(Sys, Prev), Entries, 1);
        }

        ASSERTK (Locator->Idx < 16);
        ASSERTK (StackSiz (Update) != 0);
        Entry_t *Member = StackTop (Update);
        memcpy (&Entries[Locator->Idx], Member, sizeof(Entry_t));
        StackPop (Update);
    }
    Sys->Dev->BlkWrite (Sys->Dev, DUMP_IS(Sys, Prev), Entries, 1);
}

static void _LookupSave_Child (Lookup_t *Origin, Stack_t *Child)
{
    Info_t *Sys = Origin->Sys;
    u32 Curr = Origin->Locator.Cluster;
    u32 *Idxes = MallocK (SECT_SIZ);
    Sys->Dev->BlkRead (Sys->Dev, TAB_IS(Sys, Curr), Idxes, 1);

    while (true)
    {
        Entry_t *Entries = MallocK (SECT_SIZ);
        Sys->Dev->BlkRead (Sys->Dev, DUMP_IS(Sys, Curr), Entries, 1);

        for (int i = 0 ; i < SECT_SIZ / sizeof(Entry_t) ; i++)
        {
            if (!StackEmpty (Child)) {
                void *Member = StackTop (Child);
                memcpy (&Entries[i], Member, sizeof(Entry_t));
                StackPop (Child);
            } else {
                memset (&Entries[i], 0, sizeof(Entry_t));
            }
        }
        Sys->Dev->BlkWrite (Sys->Dev, DUMP_IS(Sys, Curr), Entries, 1);

        if (Idxes[Curr % FAT_ALOC_NR] == FAT_EOF)
            goto Brk;
        if (!(ALIGN_DOWN(Curr, FAT_ALOC_NR) <= Idxes[Curr % FAT_ALOC_NR]
              && Idxes[Curr % FAT_ALOC_NR] < ALIGN_DOWN(Curr, FAT_ALOC_NR) + FAT_ALOC_NR))
        {
            u32 New = Idxes[Curr % FAT_ALOC_NR];
            Sys->Dev->BlkRead (Sys->Dev, TAB_IS(Sys, Idxes[Curr]), Idxes, 1);
            Curr = New;
        } else {
            Curr = Idxes[Curr % FAT_ALOC_NR];
        }

        continue;
    
    Brk:
        FreeK (Entries);
        break;
    }

    FreeK (Idxes);
}

/* Erase entries which `Origin`'s list describes */
static void _LookupSave_Erase (Lookup_t *Origin)
{
    Info_t *Sys = Origin->Sys;

    u32 Prev = 0;
    List_t *Head = &Origin->Locator.List;

    /* 逆向遍历, 顺应 Stack */
    Entry_t *Entries = MallocK (SECT_SIZ);
    for (List_t *Ptr = Head->Forward ; Ptr != Head ; Ptr = Ptr->Forward) {
        Locate_t *Locator = CR (Ptr, Locate_t, List);
        if (Prev != Locator->Cluster) {
            /* 准备进入下一个扇区, 先保存 */
            if (!Prev) {
                Prev = Locator->Cluster;
                Sys->Dev->BlkRead (Sys->Dev, DUMP_IS(Sys, Prev), Entries, 1);
            }
            
            Sys->Dev->BlkWrite (Sys->Dev, DUMP_IS(Sys, Prev), Entries, 1);
        }

        ASSERTK (Locator->Idx < 16);
        ENTRY_ERASE(Entries[Locator->Idx]);
    }
    Sys->Dev->BlkWrite (Sys->Dev, DUMP_IS(Sys, Prev), Entries, 1);
}

/*
    当 Opt == LKPS_CHILD, Stack 代表 Lkp目录 的子项组成的栈
*/
static void _LookupSave (Lookup_t *Lkp, Stack_t *Stack, int Opt)
{
    ASSERTK (Opt != LKPS_NONE);

    if (Opt == LKPS_ERASE)
    {
        ASSERTK (Stack == NULL);
        _LookupSave_Erase (Lkp);
    }
    /* Write an entry created */
    else if (Opt == LKPS_CREATE)
    {
        ASSERTK (StackSiz (&Lkp->Stack) == 0);
        _LookupSave_Common (Lkp, Stack);
    }
    else if (Opt == LKPS_CHILD)
    {
        _LookupSave_Child (Lkp, Stack);
    }
    /* Overwrite the original */
    else if (Opt == LKPS_UPDATE)
    {
        if (StackSiz (&Lkp->Stack) == StackSiz (Stack)) {
            _LookupSave_Common (Lkp, Stack);
        }
        else { /* space will be taken up may be too small, in spite of the original */
            _LookupSave_Erase (Lkp);
            ASSERTK (Lkp->Node->Parent != NULL);
            Lookup_t *Prt = LKP_ORI(Lkp->Node->Parent);
            _LookupAlloc (Prt, Lkp, Stack);
            _LookupSave  (Lkp, Stack, Opt);
        }
    }
}

/* Common sync */
static void _ComSync (Node_t *Before, Node_t *After)
{
    Lookup_t *Parent = LKP_ORI(Before->Parent);
    Lookup_t *Origin = _LookupEntry (Parent, Before->Name, 0);
    Origin->Sys = Parent->Sys;

    Stack_t *Stack = _MakeEntry (After);
    _LookupSave (Origin, Stack, LKPS_UPDATE);
}

static Node_t *_Create (Node_t *Parent, char *Name, size_t Siz, int Attr, u64 Addr)
{
    Node_t *Child = MallocK (sizeof(Node_t));
    memset (Child, 0, sizeof(Node_t));

    Child->Private.Sys     = Parent->Private.Sys;
    Child->Private.SysType = Parent->Private.SysType;
    Child->Opts   = Parent->Opts;

    Child->Name = Name;
    Child->Attr = Attr;
    Child->Siz  = Siz;
    Child->Private.Addr = Addr;
    
    /* Update parent's & child's nodes */
    Child->Next = Parent->Child;
    Parent->Child = Child;
    Child->Parent = Parent;

    Child->Siz = 0;
    /* Allocate an address when it's about to be written */
    Child->Private.Addr = 0;
    if (Child->Attr & NA_DIR)
        Child->Private.Addr = _AllocPart (Child->Private.Sys);

    /* Write our child node into disk (media) */
    Lookup_t *Prt = LKP_ORI(Parent);
    Lookup_t *Lkp = LKP_ORI(Child);

    Stack_t *Stack = _MakeEntry (Child);
    _LookupAlloc (Prt, Lkp, Stack);
    _LookupSave  (Lkp, Stack, LKPS_CREATE);
    
    /* 目录开头是需要一些预设的 entry 的, 比如 '.' '..' */
    if (Child->Attr & NA_DIR) {
        Lookup_t *LkpChild = LKP_ORI(Child);
        Stack_t *Init = StackInit (NULL);

        Entry_t Head_1 = { ".          ", FA_DIR, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        Entry_t Head_2 = { "..         ", FA_DIR, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        StackPush(Init, &Head_2);
        StackPush(Init, &Head_1);
        _LookupSave (LkpChild, Init, LKPS_CHILD);
            
        StackFini (Init);
    }

    StackFini (Stack);
    return Child;
}

//
// 文件系统操作
//

_UTIL_NEXT();
_UTIL_PATH_DIR();

/*
    路径游走 , 我第一次听到这个好听的名字是在 lunaixsky 的视频里面, 这里直接沿用了!

    @param Start  父目录
    @param Path  搜索对象文件名指针
    @param Last  最后一个可以被索引的已加载的文件节点
*/
static Node_t *Fat32_PathWalk (Node_t *Start, char **Path, Node_t **Last)
{
    Info_t *Sys = Start->Private.Sys;
    Lookup_t *Ori = LKP_ORI(Start);
    Lookup_t *Lkp = Ori;

    /* Last 是最后可以被索引的已加载的文件节点 */
    *Last = Start;

    Node_t *Res = NULL;
    for (;;)
    {
        char *Nxt = _Next(*Path);

        Lkp = _LookupEntry (Lkp, *Path, 0);
        if (!Lkp)
            goto End;

        if (Nxt[0] == 0) {
            Res = Lkp->Node;
            break;
        }

        *Path = Nxt;
        *Last = Lkp->Node;

        StackFini (&Lkp->Stack);
        FreeK (Lkp);
    }

    /* 物理文件系统 "无关" 的基本信息 */
    Res->Parent = Start;
    Res->Opts = Start->Opts;
    Res->Private.Sys = Start->Private.Sys;
    Res->Private.SysType = Start->Private.SysType;
    /* 挂载到父节点 */
    Res->Next = Start->Child;
    Start->Child = Res;

End:
    return Res;
}


/* 拿到的应该是干净的 路径 , 不以 '/' 开头 */
static int Fat32_Open (Node_t *Parent, char *Path, u64 Args, Node_t **Result)
{
    int Stat = 0;

    Node_t *Ori = Parent;
    /* Last 是最后可以被索引的已加载的 文件节点 */
    Node_t *Last, *Opened;
    /* retval NULL stands for 'Not Exists' */
    Opened = Fat32_PathWalk (Ori, &Path, &Last);
    if (Opened)
    {
        if (Opened->Attr & NA_ARCHIVE && Args & O_DIR)
            Stat = -ENOTDIR;
        else if (Opened->Attr & NA_DIR && ~Args & O_DIR)
            Stat = -EISDIR;
        goto End;
    }

    if (Args & O_CREATE)
    {
        /*
           按照习惯, 默认只能在一个已经存在的目录下创建新的节点,
           但是如果 Path 不到头, 就说明此文件的父目录也不存在, 所以直接返回 NULL
        */
        if (*_Next(Path) != 0) {
            Stat = -ENOENT;
            goto End;
        }

        int Attr = 0;
        if (Args & O_DIR)
            Attr |= NA_DIR;
        else
            Attr |= NA_ARCHIVE;
        Opened = _Create (Last, Path, 0, Attr, 0);
    }

End:
    if (Stat < 0)
        Opened = NULL;
    *Result = Opened;
    return Stat;
}

/* Close a node , and delete its sub-dir -> call the public handler */
static int Fat32_Close (Node_t *This)
{
    return __VrtFs_Release (This);
}

static int _ReadContent (Node_t *This, void *Buffer, size_t ReadSiz, size_t Offset)
{
    Info_t *Sys = This->Private.Sys;

    if (ReadSiz == 0)
        return 0;
    if (Offset >= This->Siz)
        return EOF;
    
    u32 *Idxes = MallocK (Sys->Record->SecSiz);
    u32 Curr = DUMP_IC(Sys, This->Private.Addr);
    Sys->Dev->BlkRead (Sys->Dev, TAB_IS(Sys, Curr), Idxes, 1);

    size_t SectOffset = Offset / SECT_SIZ,
           ByteOffset = Offset % SECT_SIZ;
    
    size_t Real = MIN(This->Siz, Offset + ReadSiz) - Offset;
    size_t Cnt  = DIV_ROUND_UP(Real, Sys->Record->SecSiz);
    size_t Siz  = Real;

    for (int i = 0 ;  ; i++) {
        if (i >= SectOffset) {
            void *Tmp = MallocK(Sys->Record->SecSiz);
            Sys->Dev->BlkRead (
                Sys->Dev,
                DUMP_IS(Sys, Curr),
                Tmp, 1
            );
            size_t CpySiz = MIN(Siz, ByteOffset != 0 ? SECT_SIZ - ByteOffset : MIN(Siz, SECT_SIZ));
            memcpy (Buffer, Tmp + ByteOffset, CpySiz);
            FreeK (Tmp);

            ByteOffset = 0;

            Buffer += CpySiz;
            Cnt--;
            if (Cnt == 0)
                goto End;
            Siz -= CpySiz;
        }
        
        if (Idxes[Curr % FAT_ALOC_NR] == FAT_EOF)
            goto End;
        if (!(ALIGN_DOWN(Curr, FAT_ALOC_NR) <= Idxes[Curr % FAT_ALOC_NR]
              && Idxes[Curr % FAT_ALOC_NR] < ALIGN_DOWN(Curr, FAT_ALOC_NR) + FAT_ALOC_NR))
        {
            u32 New = Idxes[Curr % FAT_ALOC_NR];
            Sys->Dev->BlkRead (
                Sys->Dev,
                TAB_IS(Sys, Idxes[Curr % FAT_ALOC_NR]),
                Idxes, 1
            );
            Curr = New;
        } else {
            Curr = Idxes[Curr % FAT_ALOC_NR];
        }
    }

End:
    return Real;
}

static int Fat32_Read (Node_t *This, void *Buffer, size_t Siz, size_t Offset)
{
    return _ReadContent (This, Buffer, Siz, Offset);
}

static size_t _AllocPart (Info_t *Sys)
{
    u32 *Idxes = MallocK (SECT_SIZ);

    size_t Res = 0 , Curr = DUMP_IC(Sys, Sys->FirstDataSec);
    for ( ; ; ) {
        Sys->Dev->BlkRead (Sys->Dev, TAB_IS(Sys, Curr), Idxes, 1);

        for (int i = Curr % FAT_ALOC_NR ; i < FAT_ALOC_NR ; i++, Curr++) {
            if (Idxes[i] == 0) {
                Idxes[i] = FAT_EOF;
                Sys->Dev->BlkWrite (
                    Sys->Dev,
                    TAB_IS(Sys, Curr),
                    Idxes, 1
                );
                Res = Curr;
                goto Complete;
            }
        }
    }

Complete:
    FreeK (Idxes);
    return Res == 0 ? 0 : DUMP_IS(Sys, Res);
}

/* 拓展 This 所指向的实体, 在原有的基础上拓宽/拓宽到 Cnt 个扇区 */
static size_t _ExpandPart (Node_t *This, size_t Cnt, bool Append)
{
    Info_t *Sys = This->Private.Sys;
    u32 Curr, End;
    u32 *CurIdxes, *EndIdxes;

    if (This->Private.Addr == 0) {
        This->Private.Addr = _AllocPart (This->Private.Sys);
        if (--Cnt == 0)
            goto Exit;
    }

    ASSERTK (This->Private.Addr >= Sys->FirstDataSec);

    // TODO: starts from head
    Curr = DUMP_IC(Sys, This->Private.Addr);
    CurIdxes = MallocK (SECT_SIZ);

    Sys->Dev->BlkRead (Sys->Dev, TAB_IS(Sys, Curr), CurIdxes, 1);

    while (CurIdxes[Curr % FAT_ALOC_NR] != FAT_EOF)
    {
        if (!(ALIGN_DOWN(Curr, FAT_ALOC_NR) <= CurIdxes[Curr % FAT_ALOC_NR]
              && CurIdxes[Curr % FAT_ALOC_NR] < ALIGN_DOWN(Curr, FAT_ALOC_NR) + FAT_ALOC_NR))
        {
            u32 New = CurIdxes[Curr % FAT_ALOC_NR];
            Sys->Dev->BlkRead (
                Sys->Dev,
                TAB_IS (Sys, CurIdxes[Curr % FAT_ALOC_NR]),
                CurIdxes, 1
            );
            Curr = New;
        } else {
            Curr = CurIdxes[Curr % FAT_ALOC_NR];
        }
        
        if (!Append)
            if (--Cnt == 0)
                goto Exit;
    }
    End = Curr;
    EndIdxes = CurIdxes;

    for ( ; ; Curr++)
    {
        if (Curr % FAT_ALOC_NR == 0) {
            if (CurIdxes == EndIdxes)
                CurIdxes = MallocK (SECT_SIZ);
            Sys->Dev->BlkRead (Sys->Dev, TAB_IS(Sys, Curr), CurIdxes, 1);
        }

        /* Check if it is an available FAT1 entry */
        if (!CurIdxes[Curr % FAT_ALOC_NR]) {
            void *Zero = MallocK (SECT_SIZ);
            memset (Zero, 0, SECT_SIZ);
            Sys->Dev->BlkWrite (Sys->Dev, DUMP_IS(Sys, Curr), Zero, 1);

            EndIdxes[End % FAT_ALOC_NR] = Curr;
            if (EndIdxes != CurIdxes) {
                /* Save changes to disk */
                Sys->Dev->BlkWrite (Sys->Dev, TAB_IS(Sys, End), EndIdxes, 1);

                /* 这个空闲项已经作为一个节点链接到上一个索引, 也就是上一个结尾, 此时的结尾应该被更新 */
                FreeK (EndIdxes);    // EndIdxes != CurIdxes
                EndIdxes = CurIdxes;
            }
            End = Curr;
            
            Cnt -= 1;
        }

        if (Cnt == 0) {
            EndIdxes[End % FAT_ALOC_NR] = FAT_EOF;
            /* Save changes to disk */
            Sys->Dev->BlkWrite (
                Sys->Dev,
                TAB_IS(Sys, End),
                EndIdxes, 1
            );
            goto Complete;
        }
    }

Complete:
    FreeK (CurIdxes);
Exit:
    return Cnt;
}

/* NOTE : Do not opearte `This` in this function! */
static size_t _Expand (Node_t *This, size_t Siz)
{
    Info_t *Sys = This->Private.Sys;

    if (ALIGN_UP(This->Siz, SECT_SIZ) >= Siz)
        return Siz;

    size_t Cnt = DIV_ROUND_UP(Siz - ALIGN_UP(This->Siz, SECT_SIZ), SECT_SIZ);
    size_t Res = _ExpandPart (This, Cnt, false) * SECT_SIZ;
    ASSERTK (Res == 0);

    return Siz;
}

/*
   战术: 扩容为先
*/
static int _WriteContent (Node_t *This, void *Buffer, size_t WriteSiz, size_t Offset)
{
    if (WriteSiz + Offset >= This->Siz)
        This->Siz = _Expand (This, WriteSiz + Offset);

    Info_t *Sys = This->Private.Sys;

    u32 *Idxes = MallocK (SECT_SIZ);
    u32 Curr = DUMP_IC(Sys, This->Private.Addr);
    Sys->Dev->BlkRead (Sys->Dev, TAB_IS(Sys, Curr), Idxes, 1);

    size_t SectOffset = Offset / SECT_SIZ,
           ByteOffset = Offset % SECT_SIZ;

    size_t Cnt = DIV_ROUND_UP(WriteSiz, SECT_SIZ);
    size_t Siz = WriteSiz;
    for (size_t i = 0 ;  ; i++)
    {
        if (i >= SectOffset) {
            void *Ori = MallocK (SECT_SIZ);
            Sys->Dev->BlkRead (Sys->Dev, DUMP_IS(Sys, Curr), Ori, 1);

            /* MIN 的意义在于 : WriteSiz 可能比一个扇区的大小要小 */
            size_t CpySiz = MIN(Siz, ByteOffset != 0 ? SECT_SIZ - ByteOffset : MIN(Siz, SECT_SIZ));
            memcpy (Ori + ByteOffset, Buffer, CpySiz);
            Sys->Dev->BlkWrite (
                Sys->Dev,
                DUMP_IS(Sys, Curr),
                Ori, 1
            );
            FreeK (Ori);

            /* After using ByteOffset the first time, set it to zero */
            ByteOffset = 0;

            Buffer += CpySiz;
            Cnt--;
            if (Cnt == 0)
                goto End;
            Siz -= CpySiz;
        }
        /* Reaches the file's end */
        if (Idxes[Curr % FAT_ALOC_NR] == FAT_EOF)
            goto End;
        if (!(ALIGN_DOWN(Curr, FAT_ALOC_NR) <= Idxes[Curr % FAT_ALOC_NR]
              && Idxes[Curr % FAT_ALOC_NR] < ALIGN_DOWN(Curr, FAT_ALOC_NR) + FAT_ALOC_NR))
        {
            u32 New = Idxes[Curr % FAT_ALOC_NR];
            Sys->Dev->BlkRead (
                Sys->Dev,
                TAB_IS(Sys, Idxes[Curr % FAT_ALOC_NR]),
                Idxes, 1
            );
            Curr = New;
        } else {
            Curr = Idxes[Curr % FAT_ALOC_NR];
        }
    }

End:
    _ComSync (This, This);
    FreeK (Idxes);
    return WriteSiz;
}

static int Fat32_Write (Node_t *This, void *Buffer, size_t Siz, size_t Offset)
{
    return _WriteContent (This, Buffer, Siz, Offset);
}

#define IDX_ERASE(Idx, Stat) ({ u32 Old = (Idx) ; (Idx) = (Stat) ; Old; })

/*
    TODO : Test it

    Cut 指定是否是截断操作
*/
static void _ReleaseData (Info_t *Sys, u32 Start, bool Cut)
{
    u32 *Idxes = MallocK (SECT_SIZ);
    u32 Curr = Start;
    Sys->Dev->BlkRead (Sys->Dev, TAB_IS(Sys, Curr), Idxes, 1);

    u32 Stat = Cut ? FAT_EOF : 0;
    
    while (true)
    {
        if (Idxes[Curr % FAT_ALOC_NR] == FAT_EOF)
            break;
        
        if (!(ALIGN_DOWN(Curr, FAT_ALOC_NR) <= Idxes[Curr % FAT_ALOC_NR]
              && Idxes[Curr % FAT_ALOC_NR] < ALIGN_DOWN(Curr, FAT_ALOC_NR) + FAT_ALOC_NR))
        {
            u32 Nxt = IDX_ERASE(Idxes[Curr % FAT_ALOC_NR], Stat);
            u32 NxtIdxBlk = TAB_IS(Sys, Idxes[Nxt % FAT_ALOC_NR]);
            u32 CurIdxBlk = TAB_IS(Sys, Curr);

            Sys->Dev->BlkWrite (Sys->Dev, CurIdxBlk, Idxes, 1);
            Sys->Dev->BlkRead  (Sys->Dev, NxtIdxBlk, Idxes, 1);
            Curr = Nxt;
        } else {
            Curr = IDX_ERASE(Idxes[Curr % FAT_ALOC_NR], Stat);
        }
        Stat = 0;
    }

    FreeK (Idxes);
}

static int Fat32_Remove (Node_t *This)
{
    Info_t *Sys = This->Private.Sys;

    Lookup_t *Prt = LKP_ORI (This->Parent);
    Lookup_t *Lkp = _LookupEntry (Prt, This->Name, 0);
    _LookupSave (Lkp, NULL, LKPS_ERASE);

    /* 释放数据区 */
    _ReleaseData (Lkp->Sys, Lkp->Locator.Cluster, false);
    return Fat32_Close (This);
}

static int Fat32_Truncate (Node_t *This, size_t Offset)
{
    if (Offset > This->Siz)
        This->Siz = _Expand (This, Offset);
    else {
        Info_t *Sys = This->Private.Sys;
        /* 当 Offset == 0 时, 释放掉所有簇 */
        bool ReleaseAll = false;
        if (Offset == 0) {
            ReleaseAll = true;
            This->Private.Addr = 0;
        }
        _ReleaseData (
            Sys, DUMP_IC(Sys, This->Private.Addr),
            ReleaseAll
        );
        This->Siz = Offset;
    }
    _ComSync (This, This);

    return 0;
}

/* TODO : ret-val */
static int Fat32_MakeDir (Node_t *Parent, char *Name)
{
   Node_t *Dir = _Create (Parent, Name, 0, NA_DIR, 0);
   if (!Dir)
       return -1;

   return 0;
}

static int Fat32_ReadDir (Node_t *This)
{
    int Cnt = 0;

    Lookup_t *Lkp,
             *Prt = LKP_ORI(This);
    /* 根据索引查找之后对其进行 vfs 层面的链接 */
    while ((Lkp = _LookupEntry (Prt, NULL, Cnt)) != NULL) {
        ASSERTK (Lkp->Node != NULL);

        Node_t *Child = Lkp->Node;
        if (__VrtFs_Test (This, Child->Name, NULL, NULL)) {
            __VrtFs_Release (Child);
        } else {
            /* 物理文件系统 "无关" 的基本信息 */
            Child->Parent = This;
            Child->Opts = This->Opts;
            Child->Private.Sys = This->Private.Sys;
            Child->Private.SysType = This->Private.SysType;
            /* 挂载到父节点 */
            Child->Next = This->Child;
            This->Child = Child;
        }
        
        Cnt++;
        StackFini (&Lkp->Stack);
        FreeK (Lkp);
    }

    return Cnt;
}

