#ifndef __ARGS_H__
#define __ARGS_H__

typedef struct {
    VOID   *Data;
    UINT64 PgNum;
    UINT8  *Ptr;
} ARGS_STACK;

VOID
InitializeArgs (
        IN OUT ARGS_STACK *Stack,
        IN     UINT64     Siz
        );

EFI_STATUS
ArgsPush (
        IN     ARGS_STACK *Stack,
        IN OUT VOID       **Obj,
        IN     UINT64     Siz
        );

#endif