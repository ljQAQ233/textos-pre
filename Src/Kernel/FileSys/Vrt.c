#include "Base.h"
#include "Types.h"

#include <TextOS/Debug.h>
#include <TextOS/Memory/Malloc.h>
#include <TextOS/Console/PrintK.h>

#include <string.h>

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

static int _VrtFs_Open (Node_t *Parent, Node_t **Node, char *Path, u64 Args)
{
    /* 为 Open 扫一些障碍! */
    if (!Parent) Parent = _Root;
    if (Path[0] == '/') Parent = _Root;
    while (*Path == '/') Path++;

    Node_t *Res;
    Node_t *Start;
    if ((Res = __VrtFs_Test (Parent, Path, &Start, &Path)))
        goto Complete;

    int Stat = Start->Opts->Open (Start, Path, Args, &Res);
    if (Stat < 0) {
        Res = NULL;
        goto Complete;
    }

    /* TODO: Permission checking */

Complete:
    *Node = Res;

    return 0;
}

#include <TextOS/Assert.h>

int __VrtFs_Open (Node_t *Parent, Node_t **Node, const char *Path, u64 Args)
{
    ASSERTK (!Parent || CKDIR(Parent));

    int Res = _VrtFs_Open (Parent, Node, (char *)Path, Args);
    if (Res < 0)
        DEBUGK ("Failed to open %s (%#x) - stat : %d\n", Path, Args, Res);

    return Res;
}

int __VrtFs_Read (Node_t *This, void *Buffer, size_t Siz, size_t Offset)
{
    ASSERTK (CKFILE(This));

    int Res = This->Opts->Read (This, Buffer, Siz, Offset);
    if (Res < 0)
        DEBUGK ("Failed to read %s - stat : %d\n", This->Name, Res);

    return Res;
}
    
int __VrtFs_Write (Node_t *This, void *Buffer, size_t Siz, size_t Offset)
{
    ASSERTK (CKFILE(This));

    int Res = This->Opts->Write (This, Buffer, Siz, Offset);
    if (Res < 0)
        DEBUGK ("Failed to write %s - stat : %d\n", This->Name, Res);

    return Res;
}

int __VrtFs_Close (Node_t *This)
{
    int Res = This->Opts->Close (This);
    if (Res < 0)
        DEBUGK ("Failed to close %s - stat : %d\n", This->Name, Res);

    return Res;
}

int __VrtFs_Remove (Node_t *This)
{
    int Res = This->Opts->Remove (This);
    if (Res < 0)
        DEBUGK ("Failed to remove %s - stat : %d\n", This->Name, Res);
    return Res;
}

int __VrtFs_Truncate (Node_t *This, size_t Offset)
{
    ASSERTK (CKFILE(This));

    int Res = This->Opts->Truncate (This, Offset);
    if (Res < 0)
        DEBUGK ("Failed to truncate %s - stat : %d\n", This->Name, Res);
    return Res;
}

int __VrtFs_Release (Node_t *This)
{
    if (This->Attr & NA_DIR)
        while (This->Child)
            __VrtFs_Release (This->Child);

    /* 除去父目录项的子目录项 */
    if (This->Parent)
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

    /* 释放信息 */
    if (This->Name)
        FreeK (This->Name);

Complete:
    FreeK (This);
    return 0;
}

int __VrtFs_ReadDir (Node_t *This)
{
    ASSERTK (!This || CKDIR(This));
    if (!This) This = _Root;

    int Res = This->Opts->ReadDir (This);
    if (Res < 0)
        DEBUGK ("Read directory failed - stat : %d\n", Res);

    return Res;
}

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

extern FS_INITIALIZER ( __FsInit_Fat32);

static Regstr_t Regstr[] = {
    [FS_FAT32] = {
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
    
    char Buffer[1024] = "Hello world!";
    __VrtFs_Open (NULL, &File, "/test.txt", O_READ | O_CREATE);
    __VrtFs_Write (File, Buffer, 12, 0);
    __VrtFs_Truncate (File, 10000);
    __VrtFs_ReadDir (NULL);
}

