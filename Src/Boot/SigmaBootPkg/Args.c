/* There are many args for kernel, and some of them are
   allocated DYNAMICALLY, to make sure they are not be
   covered, we should create a stack to storage them.
   
   That is Args Stack!!!                                 */

#include <Uefi.h>
#include <Library/MemoryAllocationLib.h>

#include <Boot/Boot.h>
#include <Boot/Args.h>

VOID InitializeArgs (
        IN OUT ARGS_STACK *Stack,
        IN     UINT64     Siz
        )
{
    ASSERT (Stack != NULL);

    UINT64 PgNum = EFI_SIZE_TO_PAGES (Siz);

    Stack->PgNum = PgNum;
    Stack->Data = AllocatePages (PgNum);

    ASSERT (Stack->Data != NULL);

    Stack->Ptr = Stack->Data;
}

EFI_STATUS ArgsPush (
        IN     ARGS_STACK *Stack,
        IN OUT VOID       **Obj,
        IN     UINT64     Siz
        )
{
    ASSERT (*Obj != NULL && Siz != 0);
    ASSERT (Stack != NULL && Stack->Data != NULL && Stack->PgNum != 0);

    ASSERT ((UINT64)Stack->Data + EFI_PAGES_TO_SIZE(Stack->PgNum) - (UINT64)Stack->Ptr >= Siz);

    DEBUG ((DEBUG_INFO, "[INFO] Push data to args stack - %p, %llu\n", *Obj, Siz));

    EFI_STATUS Status = EFI_SUCCESS;

    UINT8 *Ptr = *Obj;
    UINT8 *WPtr = Stack->Ptr;

    while (Siz-- && Ptr && WPtr)
    {
        *WPtr++ = *Ptr++;
        if ((UINT64)Stack->Data + EFI_PAGES_TO_SIZE(Stack->PgNum) - (UINT64)Stack->Ptr == 0)
        {
            DEBUG ((DEBUG_INFO ,"[FAIL] Unable to push data - out of resources\n"));
            Status = EFI_OUT_OF_RESOURCES;
            break;
        }
    }

    *Obj = Stack->Ptr;
    Stack->Ptr = ALIGN_POINTER (WPtr, 8);

    return Status;
}

