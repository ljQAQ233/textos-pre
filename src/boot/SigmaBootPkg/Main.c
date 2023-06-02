#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Boot.h>
#include <Logo.h>
#include <Config.h>
#include <Graphics.h>
#include <File.h>
#include <Memory.h>
#include <Kernel.h>

typedef struct {
    UINT64 Magic;
} BOOT_CONFIG;

/* From tanyugang's Code,and I modified it,very thanks! */

EFI_STATUS ExitBootServices (
        IN     EFI_HANDLE ImageHandle,
           OUT MAP_INFO   *Info
    )
{
    EFI_STATUS Status = EFI_SUCCESS;

    Status = MemoryGetMap (Info);
    ERR_RETS(Status);

    Status = gBS->ExitBootServices (
            ImageHandle,
            Info->MapKey
        );
    ERR_RETS(Status);

    Info->MapCount = Info->MapSiz / Info->DescSiz;

    return Status;
}

EFI_STATUS EFIAPI UefiMain (
        IN EFI_HANDLE        ImageHandle,
        IN EFI_SYSTEM_TABLE  *SystemTable
        )
{
    Breakpoint();

    InitializeGraphicsServices();
    InitializeFileServices();

    InitializeConfig();

    BOOT_CONFIG *Config = AllocateZeroPool (sizeof (BOOT_CONFIG));

    CHAR16 *KernelPath = ConfigGetStringChar16 ("kernel", D_KERNEL_PATH);

    KERNEL_PAGE *KernelPages;
    EFI_PHYSICAL_ADDRESS KernelEntry;
    KernelLoad (KernelPath, &KernelEntry, &KernelPages);

    UINT64 PML4Addr;
    InitializePageTab (KernelPages, &PML4Addr);
    UpdateCr3 (PML4Addr,0);

    MAP_INFO Map;
    ExitBootServices (ImageHandle, &Map);

    Config->Magic = SIGNATURE_64('T', 'E', 'X', 'T', 'O', 'S', 'B', 'T');

    UINT64 Ret = ((UINT64 (*)(BOOT_CONFIG *))KernelEntry)(Config); // A ptr to entry and call it to get status it returned
    IGNORE (Ret);

    return EFI_SUCCESS;
}
