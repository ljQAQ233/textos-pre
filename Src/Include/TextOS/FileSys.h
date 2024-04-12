#ifndef __FILE_SYS_H__
#define __FILE_SYS_H__

#define NA_DIR     (1 << 0)
#define NA_ARCHIVE (1 << 1)
#define NA_SYSTEM  (1 << 2)
#define NA_PROTECT (1 << 3)

enum {
    O_READ  = 0x1,
    O_WRITE = 0x2,
};


struct _Node;
typedef struct _Node Node_t;

typedef struct {
    Node_t * (*Open)(Node_t *This, char *Path, u64 Args);
    int      (*Read)(Node_t *This, void *Buffer, size_t Siz, size_t Offset);
    int      (*Close)(Node_t *This);
} FsOpts_t;

struct _Node {
    char *Name;

    u64 Attr;
    u64 Siz;     // Zero for dir

    Node_t *Parent;
    struct {
        void *Sys;

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

#endif
