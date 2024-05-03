#ifndef __FILE_SYS_H__
#define __FILE_SYS_H__

enum {
    FS_FAT32,
    FS_END,
};

#define NA_DIR     (1 << 0)
#define NA_ARCHIVE (1 << 1)
#define NA_SYSTEM  (1 << 2)
#define NA_PROTECT (1 << 3)

enum {
    O_READ   = 0x1,
    O_WRITE  = 0x2,
    O_CREATE = 0x04,
    O_DIR    = 0x08,
};

struct _Node;
typedef struct _Node Node_t;

typedef struct {
    int  (*Open)(Node_t *Parent, char *Path, u64 Args, Node_t **Result);
    int  (*Close)(Node_t *This);
    int  (*Remove)(Node_t *This);
    /* 文件操作 */
    int  (*Read)(Node_t *This, void *Buffer, size_t Siz, size_t Offset);
    int  (*Write)(Node_t *This, void *Buffer, size_t Siz, size_t Offset);
    int  (*Truncate)(Node_t *This, size_t Offset);
    /* 文件夹操作 */
    int  (*ReadDir)(Node_t *This);
} FsOpts_t;

struct _Node {
    char *Name;

    u64 Attr;
    u64 Siz;     // Zero for dir

    Node_t *Parent;
    struct {
        void *Sys;
        int   SysType;

        u64 Addr;
    } Private;

    Node_t *Root;
    Node_t *Child;
    Node_t *Next;

    // u64 References;

    /* Interfaces */
    FsOpts_t *Opts;

    u64 OpenArgs;
};

extern int __VrtFs_Open (Node_t *Parent, Node_t **Node, const char *Path, u64 Args);

extern int __VrtFs_Read (Node_t *This, void *Buffer, size_t Siz, size_t Offset);

extern int __VrtFs_Write (Node_t *This, void *Buffer, size_t Siz, size_t Offset);

extern int __VrtFs_Close (Node_t *This);

extern int __VrtFs_Remove (Node_t *This);

extern int __VrtFs_Truncate (Node_t *This, size_t Offset);

extern int __VrtFs_Release (Node_t *This);

extern int __VrtFs_ReadDir (Node_t *This);

extern Node_t *__VrtFs_Test (Node_t *Start, char *Path, Node_t **Last, char **LastPath);

#endif
