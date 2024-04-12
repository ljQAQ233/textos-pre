#include "Base.h"
#include "Types.h"

#include <TextOS/Debug.h>

/*
   用来注册文件系统, 在这里列出的文件系统, 是系统支持的
*/
typedef struct {
    char      *Name;
    u8        Id;
    void      *(*Init)(Dev_t *Hd, Record_t *Mbs, Partition_t *Partition);
} Regstr_t;

static Node_t *_Root = NULL;

void __VrtFs_RootSet (Node_t *Root)
{
    if (!_Root)
        _Root = Root;
}

_UTIL_NEXT();

#include <string.h>

static bool _Cmp (char *A, char *B)
{
    size_t LenA = MIN (strlen(A), strchr (A, '/') - A);
    size_t LenB = MIN (strlen(B), strchr (B, '/') - B);

    if (LenA != LenB)
        return false;

    for (size_t i = 0 ; i < LenB ; i++)
        if (A[i] != B[i])
            return false;

    return true;
}

Node_t *__VrtFs_Test (Node_t *Start, char *Path, Node_t **Last, char **LastPath)
{
    for (Node_t *Ptr = Start->Child ; Ptr ; Ptr = Ptr->Next)
        if (_Cmp (Ptr->Name, Path))
        {
            char *Nxt = _Next(Path);
            if (Nxt[0] == '\0')
                return Ptr; 
            if (~Ptr->Attr & NA_DIR)
                break;
            return __VrtFs_Test (Ptr->Child, Nxt, Last, LastPath);
        }

    if (Last)
        *Last = Start;
    if (LastPath)
        *LastPath = Path;

    return NULL;
}

/* return-value for interfaces */
#include <TextOS/ErrNo.h>

static int _VrtFs_Open (Node_t *This, Node_t **Node, char *Path, u64 Args)
{
    /* 为 Open 扫一些障碍! */
    if (!This) This = _Root;
    if (Path[0] == '/') This = _Root;
    while (*Path == '/') Path++;

    Node_t *Res;
    Node_t *Start;
    if ((Res = __VrtFs_Test (This, Path, &Start, &Path)))
        goto Complete;

    Res = Start->Opts->Open (Start, Path, Args);
    if (Res == NULL)
        return -ENOENT;

    /* TODO: Permission checking */

Complete:
    *Node = Res;

    return 0;
}

int __VrtFs_Open (Node_t *This, Node_t **Node, const char *Path, u64 Args)
{
    int Stat = _VrtFs_Open (This, Node, (char *)Path, Args);
    if (!Stat)
        DEBUGK ("Opened %s (%#x) successfully! - size : %u\n", Path, Args, (*Node)->Siz);
    else
        DEBUGK ("Failed to open %s (%#x) - stat : %d\n", Path, Args, Stat);

    return Stat;
}

int __VrtFs_Read (Node_t *This, void *Buffer, size_t Siz, size_t Offset)
{
    int Res = This->Opts->Read (This, Buffer, Siz, Offset);
    if (Res >= 0)
        DEBUGK ("Read %s successfully! - ReadSiz : %llu\n", This->Name, Res);
    else
        DEBUGK ("Failed to read %s - stat : %d\n", This->Name, Res);

    return Res;
}

int __VrtFs_Close (Node_t *This)
{
    int Stat = This->Opts->Close (This);
    if (!Stat)
        DEBUGK ("Closed %s successfully!\n", This);
    else
        DEBUGK ("Failed to close %s - stat : %d\n", This, Stat);

    return Stat;
}

int __VrtFs_ReadDir (Node_t *Node)
{
    return 0;
}

#include <TextOS/Console/PrintK.h>

static inline void _VrtFs_ListNode (Node_t *Node, int Level)
{
    if (Node->Attr & NA_DIR) {
        PrintK ("%*q- %s\n", Level, ' ', Node->Name);
        for (Node_t *p = Node->Child ; p != NULL ; p = p->Next) {
            _VrtFs_ListNode (p, Level + 1);
        }
    } else {
        for (int i = 0 ; i < Level ; i++) PrintK (" ");
        PrintK ("%*q- %s\n", Level, ' ', Node->Name);
    }
}

void __VrtFs_ListNode (Node_t *Start)
{
    _VrtFs_ListNode (Start, 0);
}

#include <TextOS/Dev.h>
#include <TextOS/Memory/Malloc.h>

#include <string.h>

extern FS_INITIALIZER ( __FsInit_Fat32);

static Regstr_t Regstr[] = {
    {
        .Name = "Fat32",
        .Id = 0xc,
        .Init = __FsInit_Fat32
    },
    {
        .Name = "EndSym",
        .Id = 0,
        .Init = NULL
    }
};

static void _InitPartitions (Dev_t *Hd, Record_t *Record)
{
    Partition_t *Ptr = Record->Partitions;

    PrintK ("Looking for file systems...\n");

    for (int i = 0 ; i < 4 ; i++, Ptr++) {
        if (!Ptr->SysId)
            continue;

        char *Type = "None";
        void *Root = NULL;

        for (Regstr_t *Look = Regstr; Look->Id != 0 ; Look++) {
            if (Look->Id == Ptr->SysId)
                if ((Root = Look->Init (Hd, Record, Ptr)))
                {
                    Type = Look->Name;

                    __VrtFs_RootSet (Root);
                }
        }

        PrintK (" - Partition %u -> %s\n", i, Type);
    }

    __VrtFs_ListNode (_Root);
}

void InitFileSys ()
{
    Dev_t *Hd = DevLookupByType (DEV_BLK, DEV_IDE);

    Record_t *Record = MallocK (sizeof(Record_t));
    Hd->BlkRead (Hd, 0, Record, 1);

    _InitPartitions (Hd, Record);

    FreeK (Record);

    PrintK ("File system initialized!\n");
    Node_t *File;
    char Buffer[17];
    __VrtFs_Open (NULL, &File, "/EFI/Boot/BootX64.efi", O_READ);
    __VrtFs_Read (File, Buffer, 16, 0);
}

